from enum import IntEnum
from dataclasses import dataclass, field
from typing import List


class MineStatus(IntEnum):
    READY = 0
    MINING = 1
    DONE = 2


@dataclass
class RepoMeta:
    repo_id: str
    name: str
    remotes: List[str]
    branch: str
    mined: MineStatus = MineStatus.READY


@dataclass
class ConflictMergeScenario:
    repo_id: str
    ours: str
    theirs: str
    base: str
    merged: str
    conflicts: List[str]

    @property
    def ms_id(self):
        return f"{self.ours[:8]}-{self.theirs[:8]}-{self.base[:8]}-{self.merged[:8]}"


class ConflictBlock:
    index: int
    ours: str
    theirs: str
    base: str
    merged: str
    labels: List[str]

    start_line: int
    end_line: int


@dataclass
class ConflictSource:
    repo_id: str
    ms_id: str
    ours: str
    theirs: str
    base: str
    merged: str
    conflict: str

    conflicts: List[ConflictBlock]
