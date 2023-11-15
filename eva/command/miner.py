from dataclasses import asdict
import logging
from pathlib import Path
from typing import List
import pygit2
from command.model import MineStatus, RepoMeta

import config
from serializer.mongo import get_mongo_client
from utils.gitservice import get_conflict_source, get_repo, next_conflict_merge_scenario

logger = logging.getLogger()


def mine_repos_conflicts(repos: List[str], ms_limit: int):
    for repo in repos:
        mine_repo_conflicts((repo, ms_limit))


def mine_repo_conflicts(repo_workset: tuple[pygit2.Repository, int]):
    repo_path, ms_limit = repo_workset
    repo = get_repo(repo_path)
    repo_path = Path(repo.path)
    repo_name = repo_path.parent.name

    mongo_client = get_mongo_client()

    db = mongo_client[config.mongo_config.EVA_DB]
    project_collection = db[config.mongo_config.PROJECT_COLLECTION]
    ms_collection = db[config.mongo_config.MS_COLLECTION]
    cs_collection = db[config.mongo_config.CS_COLLECTION]

    meta = project_collection.find_one({"name": repo_name})

    if meta:
        mined = meta["mined"]
        if isinstance(mined, int):
            mine_status = MineStatus(meta["mined"])

            if mine_status in [MineStatus.DONE, MineStatus.MINING]:
                logger.info(
                    f"mine status of ${repo_name} is {mine_status.name}, skipped"
                )
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

    logger.info(f"mining repo {repo_name}")
    repo_meta = RepoMeta(
        repo_id="",
        name=repo_name,
        remotes=[remote.url for remote in repo.remotes],
        branch=repo.head.shorthand,
        mined=MineStatus.MINING,
    )
    repo_meta.repo_id = str(
        project_collection.insert_one(repo_meta.to_mongo()).inserted_id
    )

    for ms in next_conflict_merge_scenario(repo, ms_limit):
        ms.repo_id = repo_meta.repo_id
        logger.info(f"a conflict merge scenario found: {ms}")
        ms_collection.insert_one(ms.to_mongo())
        sources = []
        for conflict in ms.conflicts:
            cs = get_conflict_source(repo, ms, *conflict)
            if not cs:
                continue
            cs.repo_id = repo_meta.repo_id
            sources.append(cs.to_mongo())
        cs_collection.insert_many(sources)
