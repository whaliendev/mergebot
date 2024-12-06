#!/bin/env python3

import json
import logging
import os
import re
import subprocess
import time
from typing import Dict, Generator, Iterable, List, Optional, Tuple

import urllib.request
import urllib.parse

import log as _
from argparse import ArgumentParser
from utils import gitservice
from utils.fileop import expand_path, remove_cpp_comments
from utils.git import get_conflict_files, git_checkout, git_merge, git_merge_abort, git_stash_changes, clear_git_stash
from os.path import join

logger = logging.getLogger()

C_EXTENSIONS = {".c", ".cpp", ".h", ".hpp", ".cc", ".cxx", ".hxx", ".C", ".cx"}
HEADERS = {
    "User-Agent": "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/113.0.0.0 Safari/537.36",
    "Content-Type": "application/json",
    "Accept": "application/json",
}
MERGEBOT_CPP_BASE = "http://127.0.0.1:18080/api/sa"
RESOLVE_RETRY = 12
RESOLVE_INTERVAL = 10


class MergeScenario:
    merged: str
    ours: str
    theirs: str
    base: str
    files: List[str]

    def __init__(self, merged, ours, theirs, base) -> None:
        self.merged = merged
        self.ours = ours
        self.theirs = theirs
        self.base = base
        self.files = []

    def __repr__(self):
        return f"MergeScenario(merged={self.merged}, ours={self.ours}, theirs={self.theirs}, base={self.base}, files={self.files})"

    def id(self):
        """ours-theirs-base"""
        return f"{self.ours[:8]}-{self.theirs[:8]}-{self.base[:8]}"


class ConflictSource:
    file_path: str
    merged_line_count: int
    mergebot_line_count: int
    diff_line_count: int

    def __init__(
        self, file_path, merged_line_count, mergebot_line_count, diff_line_count
    ) -> None:
        self.file_path = file_path
        self.merged_line_count = merged_line_count
        self.mergebot_line_count = mergebot_line_count
        self.diff_line_count = diff_line_count

    def __repr__(self):
        return f"ConflictSource(file_path={self.file_path}, merged_line_count={self.merged_line_count}, mergebot_line_count={self.mergebot_line_count}, diff_line_count={self.diff_line_count})"


class MergeScenarioStatistics:
    conflict_sources: List[ConflictSource]

    total_diff_line_count: int
    total_merged_line_count: int
    total_mergebot_line_count: int

    def __init__(
        self,
        conflict_sources,
        total_diff_line_count,
        total_merged_line_count,
        total_mergebot_line_count,
    ) -> None:
        self.conflict_sources = conflict_sources
        self.total_diff_line_count = total_diff_line_count
        self.total_merged_line_count = total_merged_line_count
        self.total_mergebot_line_count = total_mergebot_line_count

    def precision(self):
        return (
            self.total_mergebot_line_count - self.total_diff_line_count
        ) / self.total_mergebot_line_count

    def recall(self):
        return (
            self.total_mergebot_line_count - self.total_diff_line_count
        ) / self.total_merged_line_count

    def __repr__(self) -> str:
        return f"MergeScenarioStatistics(conflict_sources={self.conflict_sources}, total_diff_line_count={self.total_diff_line_count}, total_merged_line_count={self.total_merged_line_count}, total_mergebot_line_count={self.total_mergebot_line_count}, precision={self.precision()}, recall={self.recall()}"


class EvaluateEncoder(json.JSONEncoder):
    def default(self, o):
        if isinstance(o, MergeScenarioStatistics):
            return {
                "conflict_sources": o.conflict_sources,
                "total_diff_line_count": o.total_diff_line_count,
                "total_merged_line_count": o.total_merged_line_count,
                "total_mergebot_line_count": o.total_mergebot_line_count,
                "precision": o.precision(),
                "recall": o.recall(),
            }
        elif isinstance(o, ConflictSource):
            return {
                "file_path": o.file_path,
                "merged_line_count": o.merged_line_count,
                "mergebot_line_count": o.mergebot_line_count,
                "diff_line_count": o.diff_line_count,
            }
        elif isinstance(o, MergeScenario):
            return {
                "merged": o.merged,
                "ours": o.ours,
                "theirs": o.theirs,
                "base": o.base,
                "files": o.files,
            }
        else:
            return super().default(o)


def object_hook(obj):
    if "conflict_sources" in obj:
        return MergeScenarioStatistics(
            conflict_sources=obj["conflict_sources"],
            total_diff_line_count=obj["total_diff_line_count"],
            total_merged_line_count=obj["total_merged_line_count"],
            total_mergebot_line_count=obj["total_mergebot_line_count"],
        )
    elif "file_path" in obj:
        return ConflictSource(
            file_path=obj["file_path"],
            merged_line_count=obj["merged_line_count"],
            mergebot_line_count=obj["mergebot_line_count"],
            diff_line_count=obj["diff_line_count"],
        )
    elif "merged" in obj:
        return MergeScenario(
            merged=obj["merged"],
            ours=obj["ours"],
            theirs=obj["theirs"],
            base=obj["base"],
            files=obj["files"],
        )
    return obj


class EvaluateDecoder(json.JSONDecoder):
    def __init__(self, *args, **kwargs):
        super().__init__(object_hook=object_hook, *args, **kwargs)


def is_valid_directory(parser: ArgumentParser, arg: str) -> str:
    if not os.path.isdir(arg):
        parser.error(f"The directory {arg} does not exist!")
    else:
        return arg


def is_valid_file(parser: ArgumentParser, arg: str) -> str:
    if not os.path.isfile(arg):
        parser.error(f"The file {arg} does not exist!")
    else:
        return arg


def mine_merged_list(
    repo_path: str, merged_list: List[str]
) -> Generator[MergeScenario, None, None]:
    raise NotImplementedError


def get_conflicts_count(conflict_collection: Iterable) -> int:
    conflict_count = 0
    c_cpp_count = 0
    for conflict in conflict_collection:
        ends_with = lambda ext, commit: (
            False if not commit else commit.path.endswith(ext)
        )
        if any(ends_with(ext, commit) for commit in conflict for ext in C_EXTENSIONS):
            c_cpp_count += 1
        conflict_count += 1
    return c_cpp_count


def mine_git_repo(
    repo_path: str, limit: int, conflicts_count: int
) -> Generator[MergeScenario, None, None]:
    repo = gitservice.get_repo(repo_path)
    try:
        head_commit = repo.head.target

        revwalk = repo.walk(head_commit)
        yield_count = 0
        for commit in revwalk:
            parents = commit.parents
            if len(parents) < 2:
                continue

            merged_index = repo.merge_commits(
                parents[0], parents[1], flags={"fail_on_conflict": False}
            )

            if (
                merged_index.conflicts
                and get_conflicts_count(merged_index.conflicts) >= conflicts_count
            ):
                yield_count += 1
                merge_base = repo.merge_base(parents[0].oid, parents[1].oid)
                base = merge_base.hex if merge_base else ""
                conflict_merge_scenario = MergeScenario(
                    merged=commit.hex,
                    ours=parents[0].hex,
                    theirs=parents[1].hex,
                    base=base,
                )

                if yield_count <= limit:
                    yield conflict_merge_scenario
                else:
                    break
    except Exception as e:
        logger.error(f"could not get next conflict merge scenario: {e}")
        raise e


def read_file(file_path: str) -> List[str]:
    if not os.path.exists(file_path):
        logger.error(f"file {file_path} does not exist")
        return []

    with open(file_path, "r") as f:
        return f.readlines()


def count_file_lines(file_path: str) -> int:
    return len(read_file(file_path))


def do_mergebot_request(
    endpoint: str, data: Dict
) -> Tuple[int, Optional[Optional[Dict]]]:
    encoded_data = json.dumps(data).encode("utf-8")
    req = urllib.request.Request(
        f"{MERGEBOT_CPP_BASE}{endpoint}", encoded_data, HEADERS
    )

    class NonRaisingHTTPErrorProcessor(urllib.request.HTTPErrorProcessor):
        def http_response(self, request, response):
            return response

        https_response = http_response

    opener = urllib.request.build_opener(NonRaisingHTTPErrorProcessor)
    with opener.open(req) as resp:
        if resp.status == 200:
            data = json.loads(resp.read().decode("utf-8"))
            return resp.status, data.get("data")
        else:
            data = json.loads(resp.read().decode("utf-8"))
            logger.error(
                f"fail to request {endpoint} with data: \n{json.dumps(data, indent=2)}\n\n"
            )
            logger.error(f"status code returned: {resp.status}")
            err_msg = data.get("msg", "unknown error")
            logger.error(f"possible error msg: {err_msg}")
            return resp.status, None


def start_and_wait_mergebot_cpp(
    repo_path, ms: Dict[str, str], compdb_path: Optional[str], first_cpp_file: str
) -> bool:
    request_data = {
        "path": repo_path,
        "ms": ms,
    }
    if compdb_path:
        request_data["compile_db_path"] = compdb_path
    status, _ = do_mergebot_request("/ms", request_data)
    if status != 200:
        logger.error(
            f"failed to request mergebot.cpp with request data: \n{request_data}, status: {status}"
        )
        exit(1)

    # wait for mergebot.cpp to finish
    if "compile_db_path" in request_data:
        del request_data["compile_db_path"]
    request_data["file"] = first_cpp_file
    retry_count = 0
    pending = True
    while retry_count <= RESOLVE_RETRY and pending:
        status, data = do_mergebot_request("/resolve", request_data)
        if status != 200:
            logger.error(
                f"failed to request mergebot.cpp with \n{request_data}, status: {status}"
            )
            exit(1)

        # 200 OK
        pending = data.get("pending", True)
        if pending:
            time.sleep(RESOLVE_INTERVAL)
            retry_count += 1
        else:
            logger.info(f"mergebot.cpp finished with {ms}")
            break

    if pending:
        logger.error(
            f"mergebot.cpp TIMEOUT to return with \n{request_data} after {RESOLVE_INTERVAL * RESOLVE_RETRY} seconds"
        )
        return False
    return True


def get_diff_lines(
    merged_file: str, mergebot_file: str, ms_out_dir: str, filename: str
) -> List[str]:
    """
    use git diff --ignore-cr-at-eol --ignore-all-space --ignore-blank-lines --ignore-space-change --no-index -U0 mergebot_file merged_file'
    to get the diff lines and write it to ms_out_dir/filename.diff

    cat ms_out_dir/filename.diff | grep -E '^\+|^-|^\s*@@' and split it to diff_lines
    """
    diff_file = os.path.join(ms_out_dir, f"{filename}.diff")
    with open(diff_file, "w") as f:
        subprocess.run(
            [
                "git",
                "diff",
                "--ignore-cr-at-eol",
                "--ignore-all-space",
                "--ignore-blank-lines",
                "--ignore-space-change",
                "--no-index",
                "-U0",
                mergebot_file,
                merged_file,
            ],
            stdout=f,
        )

    diff_lines = []
    with open(diff_file, "r") as f:
        for line in f:
            if line.startswith("+") or line.startswith("-") or line.startswith("@@"):
                diff_lines.append(line)

    return diff_lines


class DiffHunk:
    deletion_list: List[str]
    addition_list: List[str]
    atat_line: str

    def __init__(self) -> None:
        self.deletion_list = []
        self.addition_list = []
        self.atat_line = ""


def write_diff_hunks_to_file(
    first_line: str,
    second_line: str,
    diff_hunks: List[DiffHunk],
    ms_out_dir: str,
    filename: str,
):
    diff_hunks_file = os.path.join(ms_out_dir, f"{filename}_processed.diff")
    with open(diff_hunks_file, "w") as f:
        f.write(first_line)
        f.write(second_line)
        for diff_hunk in diff_hunks:
            f.write(diff_hunk.atat_line)
            f.write("".join(diff_hunk.deletion_list))
            f.write("".join(diff_hunk.addition_list))


def analyze_diff_lines(diff_lines: List[str], ms_out_dir: str, filename: str) -> int:
    """analyze the diff_lines to calculate the diff_line_count"""
    diff_line_count = 0
    start = 0
    if (
        len(diff_lines) >= 2
        and diff_lines[0].startswith("---")
        and diff_lines[1].startswith("+++")
    ):
        start = 2
        first_line = diff_lines[0]
        second_line = diff_lines[1]

    DiffHunkList = []
    diff_line_count = 0
    i = start
    while i < len(diff_lines):
        if diff_lines[i].startswith("@@"):
            diff_hunk = DiffHunk()
            diff_hunk.atat_line = diff_lines[i]
            i += 1
            while i < len(diff_lines) and not diff_lines[i].startswith("@@"):
                if diff_lines[i].startswith("-"):
                    diff_hunk.deletion_list.append(diff_lines[i])
                elif diff_lines[i].startswith("+"):
                    diff_hunk.addition_list.append(diff_lines[i])
                i += 1
            processor = lambda lst: re.sub(r'\s+', '', '\n'.join(lst)).replace('+', '').replace('-', '')
            deletion = processor(diff_hunk.deletion_list)
            addition = processor(diff_hunk.addition_list)
            if deletion != addition:
                diff_line_count += len(diff_hunk.deletion_list)
                diff_line_count += len(diff_hunk.addition_list)
                DiffHunkList.append(diff_hunk)

    write_diff_hunks_to_file(
        first_line, second_line, DiffHunkList, ms_out_dir, filename
    )

    return diff_line_count


def evaluate_on_merge_scenario(
    repo_path: str,
    conflict_merge_scenario: MergeScenario,
    repo_out_path: str,
    remove_comments: bool,
    compdb_path: Optional[str] = None,
) -> None:
    """evaluate_on_merge_scenario evaluates the mergebot.cpp on a conflict merge scenario
    1. git checkout conflict_merge_scenario.ours
    2. git merge conflict_merge_scenario.theirs
    3. git diff --name-only --diff-filter=U to fill conflict_merge_scenario.files
    for each conflict file in conflict_merge_scenario.files:
        1. git show conflict_merge_scenario.merged:conflict_file > merged_file
        2. request merged contents from mergebot.cpp > mergebot_file
        3. calculate merged_line_count, mergebot_line_count, diff_line_count
    4. calculate the precision, recall of this conflict merge scenario, dump to a file
    5. dump overall conflict merge scenario statistics to a file
    6. abort the merge
    """
    git_checkout(repo_path, conflict_merge_scenario.ours)
    git_merge(repo_path, conflict_merge_scenario.theirs)
    conflict_merge_scenario.files = get_conflict_files(repo_path)

    ms_out_path = os.path.join(repo_out_path, conflict_merge_scenario.id())
    if not os.path.exists(ms_out_path):
        os.makedirs(ms_out_path)

    first_cpp_file = next(
        (
            file
            for file in conflict_merge_scenario.files
            if file.endswith(tuple(C_EXTENSIONS))
        ),
        conflict_merge_scenario.files[0],
    )
    success = start_and_wait_mergebot_cpp(
        repo_path,
        {
            "ours": conflict_merge_scenario.ours,
            "theirs": conflict_merge_scenario.theirs,
        },
        compdb_path,
        first_cpp_file,
    )
    if not success:
        logger.error(f"mergebot.cpp failed to start with {conflict_merge_scenario}")
        os.rmdir(ms_out_path)
        git_merge_abort(repo_path)
        git_stash_changes(repo_path)
        clear_git_stash(repo_path)
        time.sleep(10)
        return
    conflict_sources = []
    for conflict_file in conflict_merge_scenario.files:
        if not conflict_file.endswith(tuple(C_EXTENSIONS)):
            continue

        filename, file_ext = os.path.splitext(conflict_file)
        file_name_escaped = filename.replace("/", "@")
        merged_file = os.path.join(ms_out_path, f"{file_name_escaped}.merged{file_ext}")
        mergebot_file = os.path.join(
            ms_out_path, f"{file_name_escaped}.mergebot{file_ext}"
        )

        subprocess.run(
            ["git", "show", f"{conflict_merge_scenario.merged}:{conflict_file}"],
            cwd=repo_path,
            stdout=open(merged_file, "w"),
        )

        # request merged contents from mergebot.cpp
        status, data = do_mergebot_request(
            "/resolve",
            {
                "path": repo_path,
                "file": conflict_file,
                "ms": {
                    "ours": conflict_merge_scenario.ours,
                    "theirs": conflict_merge_scenario.theirs,
                },
            },
        )
        assert (
            status == 200 and data is not None
        ), f"failed to request mergebot.cpp with {conflict_merge_scenario}"
        if "merged" not in data:
            logger.error(
                f"mergebot.cpp failed to resolve {conflict_file} in {conflict_merge_scenario}"
            )
            continue

        with open(mergebot_file, "w") as f:
            f.write("\n".join(data["merged"]))

        merged_to_diff = merged_file
        mergebot_to_diff = mergebot_file
        if remove_comments:
            merged_to_diff = join(
                ms_out_path, f"{file_name_escaped}.no_comments_merged{file_ext}"
            )
            mergebot_to_diff = join(
                ms_out_path, f"{file_name_escaped}.no_comments_mergebot{file_ext}"
            )
            remove_cpp_comments(merged_file, merged_to_diff)
            remove_cpp_comments(mergebot_file, mergebot_to_diff)

        diff_lines = get_diff_lines(
            merged_to_diff, mergebot_to_diff, ms_out_path, file_name_escaped
        )
        diff_line_cnt = analyze_diff_lines(diff_lines, ms_out_path, file_name_escaped)
        conflict_sources.append(
            ConflictSource(
                file_path=conflict_file,
                merged_line_count=count_file_lines(merged_file),
                mergebot_line_count=count_file_lines(mergebot_file),
                diff_line_count=diff_line_cnt,
            )
        )
    total_diff_line_count = sum(
        conflict_source.diff_line_count for conflict_source in conflict_sources
    )
    total_merged_line_count = sum(
        conflict_source.merged_line_count for conflict_source in conflict_sources
    )
    total_mergebot_line_count = sum(
        conflict_source.mergebot_line_count for conflict_source in conflict_sources
    )
    merge_scenario_statistics = MergeScenarioStatistics(
        conflict_sources=conflict_sources,
        total_diff_line_count=total_diff_line_count,
        total_merged_line_count=total_merged_line_count,
        total_mergebot_line_count=total_mergebot_line_count,
    )
    with open(os.path.join(ms_out_path, "statistics.json"), "w", encoding="utf-8") as f:
        json.dump(merge_scenario_statistics, f, cls=EvaluateEncoder, indent=2)
    git_merge_abort(repo_path)
    git_stash_changes(repo_path)
    clear_git_stash(repo_path)
    # print(merge_scenario_statistics)
    logger.info(f"Merge Scenario Statistics: {merge_scenario_statistics}")
    time.sleep(10)


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(
        description="Evaluate mergebot.cpp on specific git repositories"
    )
    parser.add_argument(
        "-l",
        "--limit",
        help="Number of merge scenarios to evaluate",
        type=int,
        default=10,
    )
    parser.add_argument(
        "-c",
        "--conflicts-count",
        help="Number of conflict files a conflict merge scenario must have",
        type=int,
        default=1,
    )
    parser.add_argument(
        "--merged",
        help="Path to conflict merge scenarios file",
        type=lambda x: is_valid_file(parser, x),
    )
    parser.add_argument(
        "--compdb",
        help="Path to compile_commands.json",
        type=lambda x: is_valid_file(parser, x),
    )
    parser.add_argument(
        "-o",
        "--output",
        help="Root Directory to output intermediate files",
        type=str,
        default="./output",
    )
    parser.add_argument(
        "--no_remove_comments",
        help="Do not remove comments when evaluating mergebot.cpp",
        action="store_true",
    )
    parser.add_argument(
        "repo_path",
        help="Path to git repository",
        type=lambda x: is_valid_directory(parser, x),
    )
    args = parser.parse_args()

    # argument validation
    if args.merged and args.limit != parser.get_default("limit"):
        parser.error("Cannot specify --merged and --limit at the same time")
    try:
        gitservice.get_repo(args.repo_path)
    except ValueError as e:
        parser.error(e)

    # print(args)
    logger.debug(f"Arguments: {args}")
    out_dir = expand_path(args.output)
    if not os.path.exists(out_dir):
        os.makedirs(out_dir)
    logger.debug(f"Output Directory: {out_dir}")
    repo_out_path = os.path.join(out_dir, os.path.basename(args.repo_path))
    if not os.path.exists(repo_out_path):
        os.makedirs(repo_out_path)

    if args.merged:
        merged_list = []
        with open(args.merged, "r") as f:
            for line in f:
                merged_hash = line.strip()
                assert len(merged_hash) == 40 and all(
                    c in "0123456789abcdef" for c in merged_hash
                ), f"Invalid hash: {merged_hash}"
                merged_list.append(line.strip())

        if len(merged_list) == 0:
            logger.error(f"No merge scenarios found in {args.merged}")
            exit(1)

        for conflict_merge_scenario in mine_merged_list(args.repo_path, merged_list):
            logger.debug(f"Conflict Merge Scenario: {conflict_merge_scenario}")
            evaluate_on_merge_scenario(
                args.repo_path,
                conflict_merge_scenario,
                repo_out_path,
                not args.no_remove_comments,
                args.compdb,
            )
    else:
        logger.debug(f"Limit: {args.limit}")
        logger.debug(f"Conflict Count: {args.conflicts_count}")

        for conflict_merge_scenario in mine_git_repo(
            args.repo_path, args.limit, args.conflicts_count
        ):
            logger.debug(f"Conflict Merge Scenario: {conflict_merge_scenario}")
            evaluate_on_merge_scenario(
                args.repo_path,
                conflict_merge_scenario,
                repo_out_path,
                not args.no_remove_comments,
                args.compdb,
            )
