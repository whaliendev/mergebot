from enum import IntEnum
from dataclasses import dataclass
from typing import List, TypedDict


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
