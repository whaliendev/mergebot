//
// Created by whalien on 04/05/23.
//

#ifndef MB_GITSERVICE_H
#define MB_GITSERVICE_H
#include <git2.h>

#include <memory>
#include <unordered_set>

#include "mergebot/core/model/SimplifiedDiffDelta.h"
#include "mergebot/filesystem.h"

namespace mergebot {
namespace util {

class GitTree {
 public:
  GitTree(git_tree* tree) : tree(tree) {}

  ~GitTree() {
    if (tree) {
      git_tree_free(tree);
    }
  }

  // copy ctor = delete
  GitTree(const GitTree&) = delete;
  // copy assignment = delete
  GitTree& operator=(const GitTree&) = delete;

  // move ctor
  GitTree(GitTree&& other) noexcept
      : tree(std::exchange(other.tree, nullptr)) {}

  // swap idiom
  void swap(GitTree& rhs) noexcept {
    using std::swap;
    swap(tree, rhs.tree);
  }

  friend void swap(GitTree& lhs, GitTree& rhs) noexcept { lhs.swap(rhs); }

  // move assignment
  GitTree& operator=(GitTree&& other) noexcept {
    GitTree copy(std::move(other));
    copy.swap(*this);
    return *this;
  }

  git_tree* get() noexcept { return tree; }

 private:
  git_tree* tree;
};

class GitCommit {
 public:
  GitCommit(git_commit* commit) : commit(commit) {}

  ~GitCommit() {
    if (commit) {
      git_commit_free(commit);
    }
  }

  // copy ctor = delete
  GitCommit(const GitCommit&) = delete;
  // copy assignment = delete
  GitCommit& operator=(const GitCommit&) = delete;

  // move ctor
  GitCommit(GitCommit&& other) noexcept
      : commit(std::exchange(other.commit, nullptr)) {}

  // swap idiom
  void swap(GitCommit& rhs) noexcept {
    using std::swap;
    swap(commit, rhs.commit);
  }

  friend void swap(GitCommit& lhs, GitCommit& rhs) noexcept { lhs.swap(rhs); }

  // move assignment
  GitCommit& operator=(GitCommit&& other) noexcept {
    GitCommit copy(std::move(other));
    copy.swap(*this);
    return *this;
  }

  std::unique_ptr<GitTree> tree() const {
    git_tree* tree = nullptr;
    int error = git_commit_tree(&tree, commit);
    if (error < 0) return std::make_unique<GitTree>(nullptr);
    return std::make_unique<GitTree>(tree);
  }

  git_commit* get() noexcept { return commit; }

 private:
  git_commit* commit;
};

class GitRepository {
 public:
  using CommitDeleter = void (*)(git_commit*);

 public:
  GitRepository(git_repository* repo) : repo(repo) {}

  ~GitRepository() {
    if (repo != nullptr) {
      git_repository_free(repo);
    }
    git_libgit2_shutdown();
  }

  // copy ctor = delete
  GitRepository(const GitRepository&) = delete;
  // copy assignment = delete
  GitRepository& operator=(const GitRepository&) = delete;

  // move ctor
  GitRepository(GitRepository&& other) noexcept
      : repo(std::exchange(other.repo, nullptr)) {}

  // swap idiom
  void swap(GitRepository& rhs) noexcept {
    using std::swap;
    swap(repo, rhs.repo);
  }

  friend void swap(GitRepository& lhs, GitRepository& rhs) noexcept {
    lhs.swap(rhs);
  }

  // move assignment
  GitRepository& operator=(GitRepository&& other) noexcept {
    GitRepository copy(std::move(other));
    copy.swap(*this);
    return *this;
  }

  static std::unique_ptr<GitRepository> create(
      const std::string& project_path) {
    git_libgit2_init();

    std::string repo_path(project_path.c_str());
    if (!util::ends_with(repo_path, ".git")) {
      repo_path = fs::path(repo_path) / ".git";
    }

    git_repository* repo = nullptr;
    int error = git_repository_open_bare(&repo, repo_path.c_str());
    if (error < 0) {
      return nullptr;
    }
    return std::make_unique<GitRepository>(repo);
  }

  git_repository* get() noexcept { return repo; }

  std::unique_ptr<GitCommit> lookupCommit(std::string_view commit_hash) const;

  std::unique_ptr<GitCommit> lookupCommitByPrefix(
      std::string_view commit_hash) const;

 private:
  git_repository* repo;
};

// bool is_cpp_file(std::string_view path);

std::unordered_set<sa::SimplifiedDiffDelta> list_cpp_diff_files(
    std::string_view repo_path, std::string_view old_commit,
    std::string_view new_commit);

bool dump_tree_object_to(std::string_view dest, std::string_view commit_hash,
                         std::string_view repo_path);

std::optional<std::string> full_commit_hash(const std::string& hash,
                                            const std::string& project_path);

std::optional<std::string> git_merge_base(const std::string& our,
                                          const std::string& their,
                                          const std::string& project_path);
}  // namespace util
}  // namespace mergebot

#endif  // MB_GITSERVICE_H
