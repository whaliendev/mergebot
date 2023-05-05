//
// Created by whalien on 04/05/23.
//

#include "mergebot/utils/gitservice.h"

#include <git2/commit.h>
#include <git2/diff.h>
#include <git2/errors.h>
#include <git2/oid.h>
#include <git2/repository.h>
#include <git2/tree.h>
#include <spdlog/spdlog.h>

#include <unordered_set>
#include <vector>

#include "git2.h"
#include "mergebot/core/model/SimplifiedDiffDelta.h"

namespace mergebot {
namespace util {
bool isCppSource(std::string_view path) {
  using namespace std::string_view_literals;
  std::unordered_set<std::string_view> cpp_exts = {
      ".h", ".hpp", ".c", ".cc", ".cp", ".C", ".cxx", ".cpp", ".c++"};
  auto pos = path.find_last_of("."sv);
  if (pos == std::string_view::npos) return false;
  std::string_view ext = path.substr(pos);
  return cpp_exts.count(ext);
}

namespace detail {
int each_file_cb(const git_diff_delta *delta, float progress, void *payload) {
  auto diff_set =
      static_cast<std::unordered_set<sa::SimplifiedDiffDelta> *>(payload);
  if (isCppSource(delta->new_file.path)) {
    diff_set->insert(sa::SimplifiedDiffDelta{
        .OldPath = delta->old_file.path,
        .NewPath = delta->new_file.path,
        .Type = static_cast<sa::SimplifiedDiffDelta::DeltaType>(delta->status),
        .Similarity = delta->similarity});
  }
  return 0;
}
}  // namespace detail

std::unordered_set<sa::SimplifiedDiffDelta> list_cpp_diff_files(
    std::string_view repo_path, std::string_view old_commit_str,
    std::string_view new_commit_str) {
  std::unordered_set<sa::SimplifiedDiffDelta> diff_set;

  git_oid old_oid, new_oid;
  git_commit *old_commit = nullptr, *new_commit = nullptr;
  git_tree *old_tree = nullptr, *new_tree = nullptr;
  git_diff *diff = nullptr;
  git_diff_find_options options = GIT_DIFF_FIND_OPTIONS_INIT;
  options.flags = GIT_DIFF_FIND_RENAMES | GIT_DIFF_FIND_COPIES;
  git_libgit2_init();

  git_repository *repo = nullptr;
  int err = git_repository_open(&repo, repo_path.data());
  if (err < 0) goto handle;

  err = git_oid_fromstr(&old_oid, old_commit_str.data());
  err = git_commit_lookup(&old_commit, repo, &old_oid);
  err = git_commit_tree(&old_tree, old_commit);
  if (err < 0) goto handle;
  err = git_oid_fromstr(&new_oid, new_commit_str.data());
  err = git_commit_lookup(&new_commit, repo, &new_oid);
  err = git_commit_tree(&new_tree, new_commit);
  if (err < 0) goto handle;
  err = git_diff_tree_to_tree(&diff, repo, old_tree, new_tree, nullptr);
  if (err < 0) goto handle;

  err = git_diff_find_similar(diff, &options);
  err = git_diff_foreach(diff, detail::each_file_cb, nullptr, nullptr, nullptr,
                         &diff_set);
handle:
  if (err < 0) {
    const git_error *e = git_error_last();
    spdlog::error("Error {}/{}: {}", err, e->klass, e->message);
  }
  git_diff_free(diff);
  git_tree_free(old_tree);
  git_tree_free(new_tree);
  git_commit_free(old_commit);
  git_commit_free(new_commit);
  git_repository_free(repo);

  git_libgit2_shutdown();
  return diff_set;
}
}  // namespace util
}  // namespace mergebot
