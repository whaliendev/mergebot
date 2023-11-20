from dataclasses import asdict
import logging
from multiprocessing import Pool
from pathlib import Path
from typing import List
from bson import ObjectId
import pygit2
import pymongo
from command.model import MineStatus, RepoMeta

import config
from serializer.mongo import get_mongo_client
from utils.gitservice import get_conflict_source, get_repo, next_conflict_merge_scenario

logger = logging.getLogger()


def mine_repos_conflicts(repos: List[str], ms_limit: int):
    with Pool() as pool:
        pool.map(mine_repo_conflicts, [(repo, ms_limit) for repo in repos])
    # for repo in repos:
    #     mine_repo_conflicts((repo, ms_limit))


def mine_repo_conflicts(repo_workset: tuple[pygit2.Repository, int]):
    """mine conflicts of a repo

    Parameters:
    ----------
    repo_workset: tuple[pygit2.Repository, int]
        the repo to mine and the limit of merge scenarios to mine
    """
    repo_path, ms_limit = repo_workset
    repo = get_repo(repo_path)
    repo_path = Path(repo.path)
    repo_name = repo_path.parent.name

    mongo_client = get_mongo_client()

    db = mongo_client[config.mongo_config.EVA_DB]
    project_collection = db[config.mongo_config.PROJECT_COLLECTION]
    ms_collection = db[config.mongo_config.MS_COLLECTION]
    cs_collection = db[config.mongo_config.CS_COLLECTION]

    try:
        _mine_repo_conflicts_atomic(
            repo, repo_name, ms_limit, project_collection, ms_collection, cs_collection
        )
        logger.info(f"mining {repo_name} done")
    except Exception as e:
        logger.error(f"unexpected error occurs: {e}, rollback mining {repo_name}")
        repo = project_collection.find_one({"name": repo_name})
        if repo:
            oid = repo.get("_id")
            ms_collection.delete_many({"repo_id": str(oid)})
            cs_collection.delete_many({"repo_id": str(oid)})
            project_collection.delete_one({"name": repo_name})

    # with mongo_client.start_session() as session:
    #     with session.start_transaction():
    #         _mine_repo_conflicts_atomic(
    #             repo,
    #             repo_name,
    #             ms_limit,
    #             session,
    #             project_collection,
    #             ms_collection,
    #             cs_collection,
    #         )


def _mine_repo_conflicts_atomic(
    repo: pygit2.Repository,
    repo_name: str,
    ms_limit: int,
    # session: pymongo.client_session.ClientSession,
    project_collection: pymongo.collection.Collection,
    ms_collection: pymongo.collection.Collection,
    cs_collection: pymongo.collection.Collection,
):
    """mine conflicts of a repo in an atomic transaction

    Parameters:
    ----------
    repo: pygit2.Repository
        the repository to mine
    repo_name: str
        the name of the repository
    ms_limit: int
        the limit of merge scenarios to mine
    session: pymongo.client_session.ClientSession
        the session to use
    project_collection: pymongo.collection.Collection
        the project collection
    ms_collection: pymongo.collection.Collection
        the merge scenario collection
    cs_collection: pymongo.collection.Collection
        the conflict source collection
    """
    meta = project_collection.find_one({"name": repo_name})

    if meta:
        mined = meta["mined"]
        if isinstance(mined, int):
            mine_status = MineStatus(meta["mined"])

            if mine_status in [MineStatus.DONE, MineStatus.MINING]:
                logger.info(
                    f"mine status of {repo_name} is {mine_status.name}, skipped"
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
    project_oid = project_collection.insert_one(repo_meta.to_mongo()).inserted_id
    repo_meta.repo_id = str(project_oid)

    for ms in next_conflict_merge_scenario(repo, ms_limit):
        ms.repo_id = repo_meta.repo_id
        logger.info(f"a conflict merge scenario found: {ms}")
        ms_collection.insert_one(ms.to_mongo())
        sources = []
        for file in ms.files:
            cs = get_conflict_source(repo, ms, **file)
            if not cs:
                continue
            cs.repo_id = repo_meta.repo_id
            sources.append(cs.to_mongo())

        if sources:
            cs_collection.insert_many(sources)
        else:
            logger.warning(f"no conflict source found for {ms}")

    project_collection.update_one(
        {"_id": ObjectId(repo_meta.repo_id)}, {"$set": {"mined": MineStatus.DONE.value}}
    )
