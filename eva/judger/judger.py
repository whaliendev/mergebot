from dataclasses import dataclass
from enum import Enum
from typing import List
import re


@dataclass
class ConflictBlockModel:
    index: int
    ours: List[str]
    base: List[str]
    theirs: List[str]
    merged: List[str]
    labels: List[str]
    start_line: int
    end_line: int


class BaseJudger:
    def __init__(self, cb: ConflictBlockModel):
        self._our_lines = cb.ours
        self._base_lines = cb.base
        self._their_lines = cb.theirs
        self._merged_lines = cb.merged
        self._deflated_ours = BaseJudger.deflateCodeList(cb.ours)
        self._deflated_base = BaseJudger.deflateCodeList(cb.base)
        self._deflated_theirs = BaseJudger.deflateCodeList(cb.theirs)
        self._deflated_merged = BaseJudger.deflateCodeList(cb.merged)

    def judge(self) -> List[str]:
        pass

    @staticmethod
    def deflateCodeList(code: List[str]) -> str:
        joined = "\n".join(code)
        return re.sub(r"\s", "", joined)


class ClassifierJudger(BaseJudger):
    class Label(Enum):
        OURS = "ours"
        BASE = "base"
        THEIRS = "theirs"
        CONCAT = "concat"
        DELETION = "deletion"

        INTERLEAVE = "interleave"  # hard to auto merge

        NEWCODE = "newcode"  # almost impossible
        UNRESOLVED = "unresolved"
        UNKNOWN = "unknown"

    def judge(self) -> List[str]:
        if self.is_unresolved():
            return [ClassifierJudger.Label.UNRESOLVED.value]
        if self.accept_ours():
            return [ClassifierJudger.Label.OURS.value]
        if self.accept_theirs():
            return [ClassifierJudger.Label.THEIRS.value]
        if self.accept_base():
            return [ClassifierJudger.Label.BASE.value]
        if self.delete_all_revisions():
            return [ClassifierJudger.Label.DELETION.value]
        if self.concat_of_two_revisions():
            return [ClassifierJudger.Label.CONCAT.value]
        if self.interleave_of_revisions():
            return [ClassifierJudger.Label.INTERLEAVE.value]
        if self.new_code_introduced():
            return [ClassifierJudger.Label.NEWCODE.value]
        return [ClassifierJudger.Label.UNKNOWN.value]

    def is_unresolved(self) -> bool:
        has_conflict_line = (
            lambda x: x.startswith("<<<<<<<")
            or x.startswith("|||||||")
            or x.startswith("=======")
            or x.startswith(">>>>>>>")
        )
        return any(map(has_conflict_line, self._merged_lines))

    def accept_ours(self) -> bool:
        return self._deflated_ours == self._deflated_merged

    def accept_theirs(self) -> bool:
        return self._deflated_theirs == self._deflated_merged

    def accept_base(self) -> bool:
        return self._deflated_base == self._deflated_merged

    def concat_of_two_revisions(self) -> bool:
        # any side is empty, return false
        if not self._deflated_ours or not self._deflated_theirs:
            return False

        def concat_length_not_equal():
            return (
                (
                    len(self._deflated_ours) + len(self._deflated_theirs)
                    != len(self._deflated_merged)
                )
                and (
                    len(self._deflated_ours) + len(self._deflated_base)
                    != len(self._deflated_merged)
                )
                and (
                    len(self._deflated_theirs) + len(self._deflated_base)
                    != len(self._deflated_merged)
                )
            )

        if concat_length_not_equal():
            return False

        return (
            self._deflated_ours + self._deflated_theirs == self._deflated_merged
            or self._deflated_ours + self._deflated_base == self._deflated_merged
            or self._deflated_theirs + self._deflated_base == self._deflated_merged
            or self._deflated_theirs + self._deflated_ours == self._deflated_merged
            or self._deflated_base + self._deflated_ours == self._deflated_merged
            or self._deflated_base + self._deflated_theirs == self._deflated_merged
        )

    def new_code_introduced(self) -> bool:
        complete_set = set(self._our_lines + self._base_lines + self._their_lines)
        return any(map(lambda x: x not in complete_set, self._merged_lines))

    def delete_all_revisions(self) -> bool:
        return (
            self._deflated_ours != ""
            or self._deflated_base != ""
            or self._deflated_theirs != ""
        ) and self._deflated_merged == ""

    def interleave_of_revisions(self) -> bool:
        complete_set = set(self._our_lines + self._base_lines + self._their_lines)
        return all(map(lambda x: x in complete_set, self._merged_lines))


class MergebotJudger(BaseJudger):
    class Label(Enum):
        STYLE_RELATED = "style_related"
        BASE_UNDERUTILIZED = "base_underutilized"
        COMPLEX_CONFLICT = "complex_conflict"

    def judge(self) -> List[str]:
        if self.is_style_related():
            return [MergebotJudger.Label.STYLE_RELATED.value]
        if self.is_base_underutilized():
            return [MergebotJudger.Label.BASE_UNDERUTILIZED.value]
        return [MergebotJudger.Label.COMPLEX_CONFLICT.value]

    def is_style_related(self) -> bool:
        return self._deflated_ours == self._deflated_theirs

    def is_base_underutilized(self) -> bool:
        return (
            self._deflated_base == self._deflated_ours
            or self._deflated_base == self._deflated_theirs
        )
