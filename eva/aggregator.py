import json
import os
import sys

from os.path import join
from typing import List
from evaluate import EvaluateDecoder, MergeScenarioStatistics


def aggeragate_statistics(projects, out_dir):
    for project in projects:
        project_dir = join(out_dir, project)
        if not os.path.isdir(project_dir):
            continue

        merge_scenarios = os.listdir(project_dir)
        stats: List[MergeScenarioStatistics] = []
        for merge_scenario in merge_scenarios:
            try:
                merge_scenario_stat_file = join(
                    project_dir, merge_scenario, "statistics.json"
                )
                if not os.path.isfile(merge_scenario_stat_file):
                    continue

                with open(merge_scenario_stat_file, "r") as f:
                    merge_scenario_stat: MergeScenarioStatistics = json.load(
                        f, cls=EvaluateDecoder
                    )

                stats.append(merge_scenario_stat)
            except Exception as e:
                print(f'project: {project}, merge_scenario: {merge_scenario}, error: {e}')
                continue
        
        total_diff_lines = sum([stat.total_diff_line_count for stat in stats])
        total_merged_lines = sum([stat.total_merged_line_count for stat in stats])
        total_mergebot_lines = sum([stat.total_mergebot_line_count for stat in stats])

        precesion = (total_mergebot_lines - total_diff_lines) / total_mergebot_lines
        recall = (total_mergebot_lines - total_diff_lines) / total_merged_lines

        print(
            f"project: {project}, precesion: {precesion}, recall: {recall}, total_diff_lines: {total_diff_lines}, total_merged_lines: {total_merged_lines}, total_mergebot_lines: {total_mergebot_lines}"
        )


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: stat.py <out_dir>")
        sys.exit(1)

    out_dir = sys.argv[1]

    # all directories in out_dir are git projects
    projects = os.listdir(out_dir)

    aggeragate_statistics(projects, out_dir)
