from typing import List
from command.model import DBSummary
from rich import print as rprint
from rich.table import Table

from judger.judger import ClassifierJudger



def get_summary_of_merge_db(
    fetch_classifier_label: bool, fetch_mergebot_label: bool
) -> DBSummary:
    db_summary = DBSummary()
    db_summary.fill_overall_summary()

    if fetch_classifier_label:
        db_summary.fetch_classifier_summary()

    if fetch_mergebot_label:
        db_summary.fetch_mergebot_summary()

    return db_summary


def show_summary(
    summary: DBSummary,
    projectwise: bool,
    show_ratio: bool,
    repos: List[str],
) -> None:
    # use rich print and Table to pretty print stat

    ### show overall summary
    rprint("Overall Summary")
    rprint(f"Total Repos: {summary.repo_cnt}")
    rprint(f"Total Merge Scenarios: {summary.ms_cnt}")
    rprint(f"Total Conflict Sources: {summary.cs_cnt}")
    rprint(f"Total Conflict Blocks: {summary.cb_cnt}")

    ### show detailed summary
    if projectwise:
        pass
    else:
        # show classifier summary
        if summary.classifier_label_summary:
            classfier_table = Table(
                caption="Classifier Label Summary",
                show_header=True,
                caption_style="bold magenta",
                show_lines=True,
            )

            classfier_table.add_column("Label")
            classfier_table.add_column("Count")
            if show_ratio:
                classfier_table.add_column("Ratio")
            classifier_labels = [label.value for label in ClassifierJudger.Label]
            for label in classifier_labels:
                count = 0
                for _, label_, count_ in summary.classifier_label_summary:
                    if label == label_:
                        count += count_

                if show_ratio:
                    classfier_table.add_row(
                        label, str(count), f"{count / summary.cb_cnt:.2%}"
                    )
                else:
                    classfier_table.add_row(label, str(count))
            # print(classfier_table)
            rprint(classfier_table)

