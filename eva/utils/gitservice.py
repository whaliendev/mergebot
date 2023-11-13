import asyncio
from dataclasses import dataclass
import os
import logging
from pathlib import Path
import multiprocessing

import pygit2
from aiofile import AIOFile

from typing import Optional, List
from command.model import ConflictMergeScenario, ConflictSource
from utils.git import (
    GIT_MERGE_FILE_FAVOR_NORMAL,
    GIT_MERGE_FILE_STYLE_DIFF3,
    GitMergeFileInput,
    GitMergeFileOptions,
    git_merge_file,
    GIT_MERGE_CONFLICT_MARKER_SIZE,
)

logger = logging.getLogger(__package__)

OUR_CONFLICT_MARKER = "<" * GIT_MERGE_CONFLICT_MARKER_SIZE
BASE_CONFLICT_MARKER = "|" * GIT_MERGE_CONFLICT_MARKER_SIZE
THEIR_CONFLICT_MARKER = "=" * GIT_MERGE_CONFLICT_MARKER_SIZE
END_CONFLICT_MARKER = ">" * GIT_MERGE_CONFLICT_MARKER_SIZE


def get_repo(path: str) -> pygit2.Repository:
    """Get a pygit2.Repository object for the directory at path.

    :param path: the path to the git repository
    :return: a pygit2.Repository object
    :raises ValueError: if path does not exist or is not a valid git repository
    """
    if not os.path.exists(path) or not os.path.isdir(path):
        raise ValueError(f"path {path} does not exist or is not a directory")

    if not path.endswith(".git"):
        path_arg = path
        path = os.path.join(path, ".git")

    if not os.path.exists(path) or not os.path.isdir(path):
        raise ValueError(f"path {path_arg} is not a git repository")

    return pygit2.Repository(path)


def commit_hash_of_rev(revision: str, project_path: str) -> Optional[str]:
    """Get the commit hash of the given revision in the given project.

    :param revision: the revision to get the commit hash of
    :param project_path: the path to the project
    :return: the commit hash of the given revision, or None if the revision is invalid
    """
    try:
        repo = get_repo(project_path)
        obj = repo.revparse_single(revision)

        if obj is None:
            logger.error(
                f"could not get commit hash of revision {revision} in project {project_path}"
            )
            return None

        full_hash = obj.hex

        # obj may be a reference object, so we need to further resolve it
        if isinstance(obj, pygit2.Reference):
            full_hash = obj.peel().hex

        return full_hash
    except Exception as e:
        logger.error(
            f"could not get commit hash of revision {revision} in project {project_path}: {e}"
        )
        return None


@dataclass
class DumpTask:
    dest: str
    hash: str
    repo_path: str


def dump_tree_objects_par(dump_tasks: List[DumpTask]) -> List[bool]:
    """dump tree objects in parallel"""
    with multiprocessing.Pool() as pool:
        return pool.map(process_dump_task, dump_tasks)


def process_dump_task(dump_task: DumpTask) -> bool:
    """dump a single tree object, a sync wrapper of _dump_tree_object_to"""
    return asyncio.run(
        _dump_tree_object_to(dump_task.dest, dump_task.hash, dump_task.repo_path)
    )


async def _dump_tree_object_to(dest: str, hash: str, repo_path: str) -> bool:
    dest_path = Path(dest)
    if dest_path.exists() and not dest_path.is_dir():
        logger.error(f"destination path {dest} exists and is not a directory")
        return False

    dest_path.mkdir(parents=True, exist_ok=True)

    try:
        repo = get_repo(repo_path)

        commit_hash = commit_hash_of_rev(hash, repo_path)
        commit = repo.revparse_single(commit_hash)
        tree = repo[commit.tree_id]

        start = asyncio.get_event_loop().time()
        tasks = []
        for entry in tree:
            tasks.append(_dump_tree_entry(dest, entry, dest, repo))

        results = await asyncio.gather(*tasks)
        end = asyncio.get_event_loop().time()

        elapsed = (end - start) * 1000
        logger.info(
            f"it takes {elapsed:.4f} ms to dump commit tree object {hash} to {dest}"
        )
        return all(results)
    except Exception as e:
        logger.error(f"could not dump tree object {hash} to {dest}: {e}")
        return False


async def _dump_tree_entry(
    root: str, entry: pygit2.Object, dest_folder: str, repo: pygit2.Repository
) -> bool:
    entry_path = Path(root) / entry.name
    dest_path = Path(dest_folder) / entry_path

    object_type = entry.type
    if object_type == pygit2.GIT_OBJ_TREE:  # tree object
        if not dest_path.exists():
            try:
                dest_path.mkdir(parents=True, exist_ok=True)
            except Exception as e:  # perm or other strange bugs
                logger.error(f"could not create directory {dest_path}: {e}")
                return False

        subtree = repo[entry.id]

        tasks = []
        for subentry in subtree:
            tasks.append(_dump_tree_entry(entry_path, subentry, dest_path, repo))

        await asyncio.gather(*tasks)

    elif object_type == pygit2.GIT_OBJ_BLOB:  # blob object, includes symlinks and files
        if dest_path.exists():
            logger.debug(f"blob {entry.name} already exists at {dest_path}")
            return True

        blob = repo[entry.id]
        if entry.filemode == pygit2.GIT_FILEMODE_LINK:
            try:
                os.symlink(blob.read_raw().decode("utf-8"), dest_path)
            except Exception as e:
                logger.error(f"could not create symlink {dest_path}: {e}")
                return False

        else:
            try:
                async with AIOFile(dest_path, "wb") as afp:
                    await afp.write(blob.read_raw())
            except Exception as e:
                logger.error(f"could not write blob {entry.name} to {dest_path}: {e}")
                return False

    else:  # at this phase, we ignore other object types
        logger.error(f"unknown object type {object_type} for entry {entry.name}")
        return False

    return True


def next_conflict_merge_scenario(
    repo: pygit2.Repository, ms_limit: int
) -> ConflictMergeScenario:
    """Get the next merge scenario with conflicts in the given repository.

    :param repo: the repository to get the next merge scenario with conflicts
    :return: a MergeScenario object
    """
    head_commit = repo.head.target

    # --date-order: Show no parents before all of its children are shown, but otherwise show commits in the commit timestamp order.
    # --author-date-order: Show no parents before all of its children are shown, but otherwise show commits in the author timestamp order.
    # --topo-order: Show no parents before all of its children are shown, and avoid showing commits on multiple lines of history intermixed.
    revwalk = repo.walk(head_commit, pygit2.GIT_SORT_TOPOLOGICAL)

    conflicts_cnt = 0
    for commit in revwalk:
        parents = commit.parents
        if len(parents) < 2:
            continue

        merged_index = repo.merge_commits(
            parents[0], parents[1], flags={"fail_on_conflict": False}
        )

        if merged_index.conflicts:
            conflicts_cnt += 1
            ours = parents[0].hex
            theirs = parents[1].hex
            merge_base = repo.merge_base(parents[0].id, parents[1].id)
            base = merge_base.hex
            merged = commit.hex
            conflicts = [conflict[0].path for conflict in merged_index.conflicts]

            if conflicts_cnt <= ms_limit:
                yield ConflictMergeScenario("", ours, theirs, base, merged, conflicts)
            else:
                break


def _get_code_snippet(lines: List[str], start: int, end: int) -> List[str]:
    return lines[start + 1, end]


def _align_line_scan(
    anchor: List[str], merged: List[str], is_suffix: bool, start_index: int
) -> int:
    pivot = []
    source = []

    if not is_suffix:
        pivot.extend(reversed(anchor))
        source.extend(reversed(merged[start_index:]))
    else:
        pivot.extend(anchor)
        source.extend(merged[start_index + 1 :])

    if len(pivot) == 0 and is_suffix:
        return len(merged)
    elif len(pivot) == 0:
        return -1

    max_align = -1
    loc = 0

    for i in range(len(source)):
        if max_align > 5:
            break

        if pivot[0] == source[i]:
            j, k = i, 0
            while k < len(pivot) and j < len(source) and source[j] == pivot[k]:
                k += 1
                j += 1

            # the closer, the better
            if k >= max_align and not is_suffix:
                max_align = k
                loc = i
            elif k > max_align:
                max_align = k
                loc = i

    return len(merged) - loc - 1 if not is_suffix else start_index + loc + 1


def _read_file_content(repo: pygit2.Repository, commit_sha: str, file_path: str) -> str:
    commit = repo[commit_sha]
    tree = commit.tree
    try:
        entry = tree[file_path]
    except KeyError as e:
        logger.warning(f"file {file_path} does not exist in commit {commit_sha}")
        return ""
    blob = repo[entry.id]
    return blob.read_raw().decode()


def get_conflict_source(
    repo: pygit2.Repository, ms: ConflictMergeScenario, conflict: str
) -> ConflictSource:
    ours_content = _read_file_content(repo, ms.ours, conflict)
    theirs_content = _read_file_content(repo, ms.theirs, conflict)
    if ms.base:
        base_content = _read_file_content(repo, ms.base, conflict)
    else:
        logger.warning(f"merge scenario {ms} does not have a base commit")
    merged_content = _read_file_content(repo, ms.merged, conflict)

    our_file = GitMergeFileInput(ours_content, conflict, 0o100644)
    ancestor_file = GitMergeFileInput(base_content, conflict, 0o100644)
    their_file = GitMergeFileInput(theirs_content, conflict, 0o100644)
    merge_opts = GitMergeFileOptions(
        ms.base,
        ms.ours,
        ms.theirs,
        GIT_MERGE_FILE_FAVOR_NORMAL,
        GIT_MERGE_FILE_STYLE_DIFF3,
    )

    merge_output = git_merge_file(ancestor_file, our_file, their_file, merge_opts)

    if merge_output.automergeable:
        return None

    conflict_content = merge_output.content
    conflict_blocks = []
    index_in_file = 0
    conflict_lines = conflict_content.splitlines()
    conflict_line_cnt = len(conflict_lines)

    for index, line in enumerate(conflict_lines):
        if line.startswith(OUR_CONFLICT_MARKER):
            cb = {
                "index": index_in_file,
            }
            index_in_file += 1
            j = index
            k = index

            cb["start_line"] = index

            while j + 1 < conflict_line_cnt and not conflict_lines[j + 1].startswith(
                BASE_CONFLICT_MARKER
            ):
                j += 1
            cb["ours"] = _get_code_snippet(conflict_lines, k, j + 1)
            k = j

            while j + 1 < conflict_line_cnt and not conflict_lines[j + 1].startswith(
                THEIR_CONFLICT_MARKER
            ):
                j += 1
            cb["bases"] = _get_code_snippet(conflict_lines, k, j + 1)
            k = j

            while j + 1 < conflict_line_cnt and not conflict_lines[j + 1].startswith(
                END_CONFLICT_MARKER
            ):
                j += 1
            cb["theirs"] = _get_code_snippet(conflict_lines, k, j)
            k = j
            cb["end_line"] = j + 1
            conflict_blocks.append(cb)

    merged_lines = merged_content.splitlines()
    start_index = 0
    for cb in conflict_blocks:
        prefix = _get_code_snippet(conflict_lines, -1, cb["start_line"])
        suffix = _get_code_snippet(conflict_lines, cb["end_line"], conflict_line_cnt)
        start_line = _align_line_scan(prefix, merged_lines, False, start_index)
        start_index = start_line
        end_line = _align_line_scan(suffix, merged_lines, True, start_index)
        start_index = end_line
        cb["merged"] = _get_code_snippet(merged_lines, start_line, end_line)
        # judge strategy
        print(cb)
