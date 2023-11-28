from functools import reduce
from typing import List
from command.model import DBSummary
from rich import print as rprint
from rich.table import Table

from judger.judger import ClassifierJudger, MergebotJudger


def get_summary_of_merge_db(
    fetch_classifier_label: bool, fetch_mergebot_label: bool, langs: List[str]
) -> DBSummary:
    db_summary = DBSummary()
    db_summary.fill_repo_mapping()

    if fetch_classifier_label:
        db_summary.fetch_classifier_summary(langs)

    if fetch_mergebot_label:
        db_summary.fetch_mergebot_summary(langs)

    return db_summary


def show_overall_stats(
    classifier: bool, mergebot: bool, show_ratio: bool, langs: List[str]
):
    summary = get_summary_of_merge_db(classifier, mergebot, langs)

    repo_cnt = summary.repo_collection.count_documents({})
    ms_cnt = summary.ms_collection.count_documents({})
    cs_pipeline = [
        {"$match": DBSummary.gen_lang_filter(langs)},
        {"$group": {"_id": None, "count": {"$sum": 1}}},
    ]
    cs_stat_cursor = summary.cs_collection.aggregate(cs_pipeline)
    result = list(cs_stat_cursor)
    cs_cnt = result[0]["count"] if len(result) > 0 else 0

    cb_pipeline = [
        {"$match": DBSummary.gen_lang_filter(langs)},
        {"$unwind": "$conflicts"},
        {"$group": {"_id": None, "count": {"$sum": 1}}},
    ]
    cb_stat_cursor = summary.cs_collection.aggregate(cb_pipeline)
    result = list(cb_stat_cursor)
    cb_cnt = result[0]["count"] if len(result) > 0 else 0
    rprint("Overall Statistics")
    rprint(f"Number of Repositories: {repo_cnt}")
    rprint(f"Number of Merge Scenarios: {ms_cnt}")
    rprint(f"Number of Conflict Sources: {cs_cnt}")
    rprint(f"Number of Conflict Blocks: {cb_cnt}")

    if classifier:
        rprint()
        table = Table(title="Classifier Label Statistics", show_lines=True)
        table.add_column("Label")
        table.add_column("Count", justify="right")
        if show_ratio:
            table.add_column("Ratio", justify="right")
            cb_total = reduce(
                lambda x, y: x + y,
                [count for _, _, count in summary.classifier_label_summary],
            )
        for label in ClassifierJudger.Label:
            label_str = label.value
            count = 0
            ratio = 0.0
            for _, cb_label, cb_count in summary.classifier_label_summary:
                if cb_label == label_str:
                    count += cb_count
            if show_ratio and cb_total != 0:
                ratio = count / cb_total
                ratio_str = "{:.2%}".format(ratio)
                table.add_row(
                    label_str,
                    str(count),
                    ratio_str,
                )
            else:
                table.add_row(label_str, count)
        rprint(table)

    if mergebot:
        rprint()
        table = Table(title="Mergebot Label Statistics", show_lines=True)
        table.add_column("Label")
        table.add_column("Count", justify="right")
        if show_ratio:
            table.add_column("Ratio", justify="right")
            cb_total = reduce(
                lambda x, y: x + y,
                [count for _, _, count in summary.mergebot_label_summary],
            )
        for label in MergebotJudger.Label:
            label_str = label.value
            count = 0
            ratio = 0.0
            for _, cb_label, cb_count in summary.mergebot_label_summary:
                if cb_label == label_str:
                    count += cb_count
            if show_ratio and cb_total != 0:
                ratio = count / cb_total
                ratio_str = "{:.2%}".format(ratio)
                table.add_row(
                    label_str,
                    str(count),
                    ratio_str,
                )
            else:
                table.add_row(label_str, count)
        rprint(table)
