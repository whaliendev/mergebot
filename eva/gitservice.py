import asyncio
from dataclasses import dataclass
import os
import logging
from pathlib import Path
import multiprocessing

import pygit2
from aiofile import AIOFile

from typing import Optional, List

logger = logging.getLogger(__package__)


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
