import logging
from enum import IntEnum
from dataclasses import dataclass
from typing import Dict, List, Tuple, TypedDict
from judger.judger import ClassifierJudger, MergebotJudger

import pymongo
from config import mongo_config

from serializer.mongo import get_mongo_client


logger = logging.getLogger()


class MineStatus(IntEnum):
    READY = 0
    MINING = 1
    DONE = 2


def object_to_dict(obj):
    if hasattr(obj, "__dict__"):
        return obj.__dict__

    if isinstance(obj, list):
        return [object_to_dict(item) for item in obj]

    if isinstance(obj, dict):
        return {key: object_to_dict(value) for key, value in obj.items()}

    return obj


def object_to_mongo(obj):
    if hasattr(obj, "to_mongo"):
        return obj.to_mongo()

    if isinstance(obj, list):
        return [object_to_mongo(item) for item in obj]

    if isinstance(obj, dict):
        return {key: object_to_mongo(value) for key, value in obj.items()}

    return obj


@dataclass
class RepoMeta:
    repo_id: str
    name: str
    remotes: List[str]
    branch: str
    mined: MineStatus = MineStatus.READY

    def to_mongo(self):
        return {
            "name": self.name,
            "remotes": self.remotes,
            "branch": self.branch,
            "mined": self.mined.value,
        }

    @classmethod
    def from_mongo(cls, repo_meta):
        return cls(
            repo_id=repo_meta["_id"],  # create index on this field
            name=repo_meta["name"],
            remotes=repo_meta["remotes"],
            branch=repo_meta["branch"],
            mined=MineStatus(repo_meta["mined"]),
        )


class PathMapping(TypedDict):
    ancestor: str
    ours: str
    theirs: str


@dataclass
class ConflictMergeScenario:
    repo_id: str  # repo_id in mongo
    ours: str  # our commit id
    theirs: str  # their commit id
    base: str  # base commit id
    merged: str  # merged commit id
    files: List[PathMapping]  # file name of conflict files

    @property
    def ms_id(self):
        return f"{self.ours[:8]}-{self.theirs[:8]}-{self.base[:8]}-{self.merged[:8]}"

    def to_mongo(self):
        return {
            "repo_id": self.repo_id,  # create index on (repo_id, ms_id)
            "ms_id": self.ms_id,
            "ours": self.ours,
            "theirs": self.theirs,
            "base": self.base,
            "merged": self.merged,
            "files": [file for file in self.files],
        }

    @classmethod
    def from_mongo(cls, ms):
        return cls(
            repo_id=ms["repo_id"],
            ours=ms["ours"],
            theirs=ms["theirs"],
            base=ms["base"],
            merged=ms["merged"],
            files=[PathMapping(file) for file in ms["files"]],
        )


@dataclass
class ConflictBlock:
    index: int
    ours: str
    theirs: str
    base: str
    merged: str
    labels: List[str]

    start_line: int
    end_line: int

    def to_mongo(self):
        return {
            "index": self.index,
            "ours": self.ours,
            "theirs": self.theirs,
            "base": self.base,
            "merged": self.merged,
            "labels": self.labels,
            "start_line": self.start_line,
            "end_line": self.end_line,
        }

    @classmethod
    def from_mongo(cls, cb):
        return cls(
            index=cb["index"],
            ours=cb["ours"],
            theirs=cb["theirs"],
            base=cb["base"],
            merged=cb["merged"],
            labels=cb["labels"],
            start_line=cb["start_line"],
            end_line=cb["end_line"],
        )


@dataclass
class ConflictSource:
    repo_id: str
    ms_id: str
    paths: PathMapping  # file name
    # file content of ours, theirs, base, merged
    ours: str
    theirs: str
    base: str
    merged: str

    conflicts: List[ConflictBlock]

    def to_mongo(self):
        return {
            "repo_id": self.repo_id,  # create index on (repo_id, ms_id, conflict)
            "ms_id": self.ms_id,
            "paths": self.paths,
            "ours": self.ours,
            "theirs": self.theirs,
            "base": self.base,
            "merged": self.merged,
            "conflicts": object_to_mongo(self.conflicts),
        }

    @classmethod
    def from_mongo(cls, cs):
        return cls(
            repo_id=cs["repo_id"],
            ms_id=cs["ms_id"],
            paths=cs["paths"],
            ours=cs["ours"],
            theirs=cs["theirs"],
            base=cs["base"],
            merged=cs["merged"],
            conflicts=[
                ConflictBlock.from_mongo(conflict) for conflict in cs["conflicts"]
            ],
        )


class DBSummary:
    repo_cnt: int
    ms_cnt: int
    cs_cnt: int
    cb_cnt: int
    repo_rel_mapping: Dict[str, str]
    per_repo_summary: List[Tuple[str, int, int]]  # repo_name, ms_cnt, cs_cnt

    classifier_label_summary: List[Tuple[str, str, int]]  # repo_name, label, cnt
    mergebot_label_summary: List[Tuple[str, str, int]]  # repo_name, label, cnt

    __db: pymongo.database.Database
    __repo_collection: pymongo.collection.Collection
    __ms_collection: pymongo.collection.Collection
    __cs_collection: pymongo.collection.Collection

    def __init__(self) -> None:
        mongo_client = get_mongo_client()

        self.__db = mongo_client[mongo_config.EVA_DB]
        self.__repo_collection = self.__db[mongo_config.PROJECT_COLLECTION]
        self.__ms_collection = self.__db[mongo_config.MS_COLLECTION]
        self.__cs_collection = self.__db[mongo_config.CS_COLLECTION]

    def fill_overall_summary(self):
        self.repo_cnt = self.__repo_collection.count_documents(
            {"mined": MineStatus.DONE.value}
        )
        self.ms_cnt = self.__ms_collection.count_documents({})
        self.cs_cnt = self.__cs_collection.count_documents({})
        cb_pipeline = [
            {"$unwind": "$conflicts"},
            {"$group": {"_id": None, "conflict_count": {"$sum": 1}}},
        ]
        cb_aggregate_result = list(self.__cs_collection.aggregate(cb_pipeline))
        self.cb_cnt = (
            cb_aggregate_result[0]["conflict_count"]
            if len(cb_aggregate_result) > 0
            else 0
        )

        self.repo_rel_mapping = {}
        self.per_repo_summary = []

        for repo in self.__repo_collection.find({"mined": MineStatus.DONE.value}):
            repo_name = repo["name"]
            repo_id_str = str(repo["_id"])
            ms_cnt = self.__ms_collection.count_documents({"repo_id": repo_id_str})
            cs_cnt = self.__cs_collection.count_documents({"repo_id": repo_id_str})
            self.repo_rel_mapping[repo_id_str] = repo_name
            self.per_repo_summary.append((repo_name, ms_cnt, cs_cnt))

        # print(self.repo_rel_mapping)

    def fetch_classifier_summary(self):
        if self.repo_rel_mapping is None:
            self.fill_overall_summary()

        classifier_label_mapping = {
            label.value: label for label in ClassifierJudger.Label
        }
        pipeline = [
            {"$unwind": "$conflicts"},
            {"$unwind": "$conflicts.labels"},
            {"$match": {"conflicts.labels": {"$ne": None, "$exists": True}}},
            {
                "$group": {
                    "_id": {"repo_id": "$repo_id", "label": "$conflicts.labels"},
                    "count": {"$sum": 1},
                }
            },
            {
                "$project": {
                    "repo_id": "$_id.repo_id",
                    "label": "$_id.label",
                    "count": "$count",
                    "_id": 0,
                }
            },
        ]
        result = list(self.__cs_collection.aggregate(pipeline))
        self.classifier_label_summary = [
            (
                self.repo_rel_mapping[aggregate["repo_id"]],
                aggregate["label"],
                aggregate["count"],
            )
            for aggregate in result
            if aggregate["label"] in classifier_label_mapping.keys()
        ]
        # print(f"classifier label summary: {self.classifier_label_summary}")

    def fetch_mergebot_summary(self):
        if self.repo_rel_mapping is None:
            self.fill_overall_summary()

        mergebot_label_mapping = {label.value: label for label in MergebotJudger.Label}
        pipeline = [
            {"$unwind": "$conflicts"},
            {"$unwind": "$conflicts.labels"},
            {"$match": {"conflicts.labels": {"$ne": None, "$exists": True}}},
            {
                "$group": {
                    "_id": {"repo_id": "$repo_id", "label": "$conflicts.labels"},
                    "count": {"$sum": 1},
                }
            },
            {
                "$project": {
                    "repo_id": "$_id.repo_id",
                    "label": "$_id.label",
                    "count": "$count",
                    "_id": 0,
                }
            },
        ]
        result = list(self.__cs_collection.aggregate(pipeline))
        # logger.debug(f"mergebot aggregate result: {result}")
        self.mergebot_label_summary = [
            (
                self.repo_rel_mapping[aggregate["repo_id"]],
                aggregate["label"],
                aggregate["count"],
            )
            for aggregate in result
            if aggregate["label"] in mergebot_label_mapping.keys()
        ]
