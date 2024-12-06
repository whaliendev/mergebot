import os
from pygit2 import C
import subprocess
import tempfile
from typing import Optional, List

import logging

logger = logging.getLogger(__name__)

###################
# git_merge_file_favor_t
###################
GIT_MERGE_FILE_FAVOR_NORMAL = C.GIT_MERGE_FILE_FAVOR_NORMAL
GIT_MERGE_FILE_FAVOR_OURS = C.GIT_MERGE_FILE_FAVOR_OURS
GIT_MERGE_FILE_FAVOR_THEIRS = C.GIT_MERGE_FILE_FAVOR_THEIRS
GIT_MERGE_FILE_FAVOR_UNION = C.GIT_MERGE_FILE_FAVOR_UNION

GIT_MERGE_CONFLICT_MARKER_SIZE = 7

###################
# git_merge_file_flag_t
###################
# Defaults
GIT_MERGE_FILE_DEFAULT = C.GIT_MERGE_FILE_DEFAULT
# Create standard conflicted merge files
GIT_MERGE_FILE_STYLE_MERGE = C.GIT_MERGE_FILE_STYLE_MERGE
# Create diff3-style files
GIT_MERGE_FILE_STYLE_DIFF3 = C.GIT_MERGE_FILE_STYLE_DIFF3
# Condense non-alphanumeric regions for simplified diff file
GIT_MERGE_FILE_SIMPLIFY_ALNUM = C.GIT_MERGE_FILE_SIMPLIFY_ALNUM
# Ignore all whitespace
GIT_MERGE_FILE_IGNORE_WHITESPACE = C.GIT_MERGE_FILE_IGNORE_WHITESPACE
# Ignore changes in amount of whitespace
GIT_MERGE_FILE_IGNORE_WHITESPACE_CHANGE = C.GIT_MERGE_FILE_IGNORE_WHITESPACE_CHANGE
# Ignore whitespace at end of line
GIT_MERGE_FILE_IGNORE_WHITESPACE_EOL = C.GIT_MERGE_FILE_IGNORE_WHITESPACE_EOL
# Use the "patience diff" algorithm
GIT_MERGE_FILE_DIFF_PATIENCE = C.GIT_MERGE_FILE_DIFF_PATIENCE
# Take extra time to find minimal diff
GIT_MERGE_FILE_DIFF_MINIMAL = C.GIT_MERGE_FILE_DIFF_MINIMAL
# TODO(hwa): find why
# Create zdiff3 ("zealous diff3")-style files
# GIT_MERGE_FILE_STYLE_ZDIFF3 = C.GIT_MERGE_FILE_STYLE_ZDIFF3
# Do not produce file conflicts when common regions have
# changed; keep the conflict markers in the file and accept
# that as the merge result.
# GIT_MERGE_FILE_ACCEPT_CONFLICTS = C.GIT_MERGE_FILE_ACCEPT_CONFLICTS


class GitMergeFileOptions:
    version = 1
    ancestor_label: str
    our_label: str
    their_label: str
    favor: GIT_MERGE_FILE_FAVOR_NORMAL
    flags: GIT_MERGE_FILE_DEFAULT
    marker_size = GIT_MERGE_CONFLICT_MARKER_SIZE

    def __init__(
        self,
        ancestor_label: str,
        our_label: str,
        their_label: str,
        favor: GIT_MERGE_FILE_FAVOR_NORMAL,
        flags: GIT_MERGE_FILE_DEFAULT,
    ):
        self.ancestor_label = ancestor_label
        self.our_label = our_label
        self.their_label = their_label
        self.favor = favor
        self.flags = flags


class GitMergeFileInput:
    version: int
    content: str
    path: str
    mode: int

    def __init__(self, content: str, path: str, mode: int):
        self.version = 1
        self.content = content
        self.path = path
        self.mode = mode

    @classmethod
    def plain_file(cls, content: str, path: str):
        return cls(content, path, 0o100644)

    @classmethod
    def executable_file(cls, content: str, path: str):
        return cls(content, path, 0o100755)

    @classmethod
    def symlink(cls, content: str, path: str):
        return cls(content, path, 0o120000)

    @classmethod
    def binary(cls, content: str, path: str):
        return cls(content, path, 0o100644)

    def __repr__(self):
        return f"<GitMergeFileInput content={self.content} path={self.path} mode={self.mode}>"

    def __str__(self):
        return self.content


class GitMergeFileResult:
    automergeable: bool
    path: str
    mode: int
    content: str

    def __init__(self, automergeable: bool, path: str, mode: int, content: str):
        self.automergeable = automergeable
        self.path = path
        self.mode = mode
        self.content = content

    def __repr__(self):
        return f"<GitMergeFileResult automergeable={self.automergeable} path={self.path} mode={self.mode} content={self.content}>"

    def __str__(self):
        return self.content


def git_merge_file(
    ancestor: GitMergeFileInput,
    ours: GitMergeFileInput,
    theirs: GitMergeFileInput,
    opts: GitMergeFileOptions,
) -> GitMergeFileResult:
    with tempfile.TemporaryDirectory() as tempdir:
        ancestor_path = os.path.join(tempdir, "ancestor")
        ours_path = os.path.join(tempdir, "ours")
        theirs_path = os.path.join(tempdir, "theirs")
        with open(ancestor_path, "w") as f:
            f.write(ancestor.content)
        with open(ours_path, "w") as f:
            f.write(ours.content)
        with open(theirs_path, "w") as f:
            f.write(theirs.content)
        cmd = [
            "git",
            "merge-file",
            "-L",
            ours.path,
            "-L",
            ancestor.path,
            "-L",
            theirs.path,
            "-pq",
            "--diff3",
            ours_path,
            ancestor_path,
            theirs_path,
        ]
        result = subprocess.run(cmd, capture_output=True)
        automergeable = result.returncode == 0
        return GitMergeFileResult(
            automergeable, ancestor.path, ancestor.mode, result.stdout.decode()
        )

def get_conflict_files(repo_path: str) -> Optional[List[str]]:
    result = subprocess.run(
        ["git", "diff", "--name-only", "--diff-filter=U"],
        cwd=repo_path,
        capture_output=True,
        text=True,
    )

    if result.returncode == 0:
        return result.stdout.strip().split("\n")
    else:
        logger.error("fail to get conflict files")
        return None


def git_checkout(repo_path: str, commit_hash: str) -> None:
    subprocess.run(["git", "checkout", commit_hash], cwd=repo_path, check=True)


def git_merge(repo_path: str, commit_hash: str) -> None:
    result = subprocess.run(["git", "merge", commit_hash], cwd=repo_path)

    if result.returncode != 0:
        logger.error(f"Merge failed with {commit_hash}, stderr: {result.stderr}")
    else:
        logger.info(f"Merge succeeded with {commit_hash}")


def is_in_merging(repo_path: str) -> bool:
    merge_head_file = os.path.join(repo_path, ".git", "MERGE_HEAD")
    return os.path.exists(merge_head_file)


def git_merge_abort(repo_path: str) -> None:
    subprocess.run(["git", "merge", "--abort"], cwd=repo_path, check=True)


def git_stash_changes(repo_path: str) -> None:
    subprocess.run(["git", "stash"], cwd=repo_path, check=True)


def clear_git_stash(repo_path: str) -> None:
    subprocess.run(["git", "stash", "clear"], cwd=repo_path, check=True)
