from command.model import DBSummary
from command.stat import get_summary_of_merge_db
from utils.common import timing


@timing
def _measure_count_cb(summary: DBSummary) -> int:
    count = 0
    pipeline = [
        {"$unwind": "$conflicts"},
        {"$group": {"_id": None, "count": {"$sum": 1}}},
    ]
    result = summary.cs_collection.aggregate(pipeline)

    it_index = 0
    for r in result:
        if it_index == 1:  # should be only one
            raise Exception("more than one result")
        count = r["count"]
        it_index += 1
    return count


@timing
def _measure_count_cb2(summary: DBSummary) -> int:
    total = 0
    for doc in summary.cs_collection.find():
        count = len(doc["conflicts"])
        total += count
    return total


def test_count_conflict_blocks():
    summary = get_summary_of_merge_db(True, True, [])
    count = _measure_count_cb(summary)
    count = _measure_count_cb2(summary)


if __name__ == "__main__":
    test_count_conflict_blocks()
