from enum import Enum
from functools import reduce
import logging
from typing import List
from command.model import DBSummary
from rich import print as rprint
from rich.table import Table

from judger.judger import ClassifierJudger, MergebotJudger


logger = logging.getLogger()


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


def __show_overall_label_stats(
    show_ratio: bool, label_summary: List[tuple], name: str, Label: Enum
):
    rprint()
    table = Table(title=f"{name} Label Statistics", show_lines=True)
    table.add_column("Label")
    table.add_column("Count", justify="right")
    if show_ratio:
        table.add_column("Ratio", justify="right")
        cb_total = reduce(
            lambda x, y: x + y,
            [count for _, _, count in label_summary],
        )
    for label in Label:
        label_str = label.value
        count = 0
        ratio = 0.0
        for _, cb_label, cb_count in label_summary:
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
            table.add_row(label_str, str(count))
    rprint(table)


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
        __show_overall_label_stats(
            show_ratio,
            summary.classifier_label_summary,
            "Classifier",
            ClassifierJudger.Label,
        )
    if mergebot:
        __show_overall_label_stats(
            show_ratio,
            summary.mergebot_label_summary,
            "Mergebot",
            MergebotJudger.Label,
        )


def show_projectwise_stats(
    classifier: bool,
    mergebot: bool,
    show_ratio: bool,
    langs: List[str],
    repos: List[str],
):
    summary = get_summary_of_merge_db(classifier, mergebot, langs)
    repo_mapping = summary.repo_rel_mapping  # repo_id: repo_name
    repo_name_mapping = {v: k for k, v in repo_mapping.items()}  # repo_name: repo_id

    repo_names = repo_mapping.values()

    repos_to_show = []
    for repo_name in repos:
        if repo_name not in repo_names:
            logger.error(f"{repo_name} is not in the database, skipping it")
            continue
        else:
            repos_to_show.append(repo_name)

    if repos and not repos_to_show:
        rprint("[bold red]error: [/bold red]no valid repo specified")
        exit(1)

    if not repos_to_show and not repos:
        logger.info("no repo selected, we'll show all repo projectwise statistics")
        repos_to_show = repo_names

    logger.debug(f"repos to show: {repos_to_show}")
    for repo_name in repos_to_show:
        repo_id_str = repo_name_mapping[repo_name]
        ms_cnt = summary.ms_collection.count_documents({"repo_id": repo_id_str})

        cs_pipeline = [
            {
                "$match": {
                    "$and": [DBSummary.gen_lang_filter(langs), {"repo_id": repo_id_str}]
                }
            },
            {"$group": {"_id": None, "count": {"$sum": 1}}},
        ]
        cs_stat_cursor = summary.cs_collection.aggregate(cs_pipeline)
        result = list(cs_stat_cursor)
        cs_cnt = result[0]["count"] if len(result) > 0 else 0

        cb_cnt = 0
        cb_pipeline = [
            {
                "$match": {
                    "$and": [DBSummary.gen_lang_filter(langs), {"repo_id": repo_id_str}]
                }
            },
            {"$unwind": "$conflicts"},
            {"$group": {"_id": None, "count": {"$sum": 1}}},
        ]
        cb_stat_cursor = summary.cs_collection.aggregate(cb_pipeline)
        result = list(cb_stat_cursor)
        cb_cnt = result[0]["count"] if len(result) > 0 else 0
        rprint()
        rprint(f"Projectwise Statistics of {repo_name}")
        rprint(f"Number of Merge Scenarios: {ms_cnt}")
        rprint(f"Number of Conflict Sources: {cs_cnt}")
        rprint(f"Number of Conflict Blocks: {cb_cnt}")

        if classifier:
            classifier_summary = (
                summary.classifier_label_summary
            )  # list of (repo_id, label, count)
            repo_classifier_summary = list(
                filter(lambda x: x[0] == repo_name, classifier_summary)
            )
            __show_projectwise_label_stats(
                repo_name,
                "Classifier",
                show_ratio,
                repo_classifier_summary,
                ClassifierJudger.Label,
            )

        if mergebot:
            mergebot_summary = summary.mergebot_label_summary
            repo_mergebot_summary = list(
                filter(lambda x: x[0] == repo_name, mergebot_summary)
            )
            __show_projectwise_label_stats(
                repo_name,
                "Mergebot",
                show_ratio,
                repo_mergebot_summary,
                MergebotJudger.Label,
            )


def __show_projectwise_label_stats(
    repo_name: str, name: str, show_ratio: bool, label_summary: List[tuple], Label: Enum
):
    rprint()
    table = Table(title=f"{repo_name} {name} Label Statistics", show_lines=True)
    table.add_column("Label")
    table.add_column("Count", justify="right")
    if show_ratio:
        repo_cb_cnt = reduce(
            lambda x, y: x + y, [count for _, _, count in label_summary], 0
        )
        table.add_column("Ratio", justify="right")
    for label in Label:
        label_str = label.value
        count = 0
        ratio = 0.0
        for _, cb_label, cb_count in label_summary:
            if cb_label == label_str:
                count += cb_count
        if show_ratio and repo_cb_cnt != 0:
            ratio = count / repo_cb_cnt
            ratio_str = "{:.2%}".format(ratio)
            table.add_row(
                label_str,
                str(count),
                ratio_str,
            )
        else:
            table.add_row(label_str, str(count))
    rprint(table)
