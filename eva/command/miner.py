from dataclasses import asdict, dataclass, field
from enum import IntEnum
from pathlib import Path
from typing import List
import pygit2
from multiprocessing import Pool
from command.model import ConflictSource, MineStatus, RepoMeta

import config
from env import init_env
from utils.gitservice import get_conflict_source, get_repo, next_conflict_merge_scenario


def asdict_factory(kv_pairs):
    res = {}
    for k, v in kv_pairs:
        if isinstance(v, MineStatus):
            res[k] = v.value
        else:
            res[k] = v
    return res


def mine_repos_conflicts(repos: List[str], ms_limit: int):
    # repo_worksets = [(repo, ms_limit) for repo in repos]
    for repo in repos:
        mine_repo_conflicts((repo, ms_limit))
    # with Pool() as pool:
    #     pool.map(mine_repo_conflicts, repo_worksets)


def mine_repo_conflicts(repo_workset: tuple[pygit2.Repository, int]):
    repo_path, ms_limit = repo_workset
    repo = get_repo(repo_path)
    repo_path = Path(repo.path)
    repo_name = repo_path.parent.name

    mongo_client, logger = init_env()

    db = mongo_client[config.mongo_config.EVA_DB]
    project_collection = db[config.mongo_config.PROJECT_COLLECTION]

    meta = project_collection.find_one({"name": repo_name})

    if meta:
        mined = meta["mined"]
        if isinstance(mined, int):
            mine_status = MineStatus(meta["mined"])

            if mine_status == MineStatus.DONE:
                logger.info(f"repo {repo.path} has been mined, skip")
                return
            elif mine_status == MineStatus.MINING:
                logger.info(f"repo {repo.path} is being mined, skip")
                return
            else:
                logger.info(
                    f"mine status of ${repo.path} is illegal, delete it and re-mine"
                )
                project_collection.delete_one({"name": repo.path})
        else:
            logger.info(
                f"mine status of ${repo.path} is illegal, delete it and re-mine"
            )
            project_collection.delete_one({"name": repo.path})

    logger.info(f"mining repo {repo.path}")
    repo_meta = RepoMeta(
        repo_id="",
        name=repo_name,
        remotes=[remote.url for remote in repo.remotes],
        branch=repo.head.shorthand,
        mined=MineStatus.MINING,
    )
    repo_meta.repo_id = str(
        project_collection.insert_one(
            asdict(
                repo_meta,
                dict_factory=asdict_factory,
            )
        ).inserted_id
    )

    for ms in next_conflict_merge_scenario(repo, ms_limit):
        ms.repo_id = repo_meta.repo_id

        sources = []
        for conflict in ms.conflicts:
            sources.append(
                asdict(
                    get_conflict_source(repo, ms, conflict),
                    dict_factory=asdict_factory,
                )
            )
