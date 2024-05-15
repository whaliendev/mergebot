
import os
from evaluate import evaluate_on_merge_scenario, mine_git_repo


def test_mine_git_repo():
    repo_path = '/home/whalien/codebase/cpp/protobuf'
    limit = 10
    conflicts_count = 5

    conflicts = []
    for conflict_merge_scenario in mine_git_repo(repo_path, limit, conflicts_count):
        print(conflict_merge_scenario)
        conflicts.append(conflict_merge_scenario)

    assert len(conflicts) == 10


# def test_evaluate_on_merge_scenario():
#     repo_path = '/home/whalien/codebase/cpp/protobuf'
#     limit = 10
#     conflicts_count = 5

#     out_dir = os.path.join(os.getcwd(), "out")
#     if not os.path.exists(out_dir):
#         os.makedirs(out_dir)
#     for conflict_merge_scenario in mine_git_repo(repo_path, limit, conflicts_count):
#         evaluate_on_merge_scenario(repo_path, conflict_merge_scenario, out_dir)
