//
// Created by whalien on 04/05/23.
//

#include "mergebot/utils/gitservice.h"

#include <git2.h>
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_for.h>
#include <spdlog/spdlog.h>

#include <fstream>
#include <unordered_set>

#include "mergebot/core/model/SimplifiedDiffDelta.h"
#include "mergebot/filesystem.h"

namespace mergebot {
namespace util {
bool is_cpp_file(std::string_view path) {
  using namespace std::string_view_literals;
  std::unordered_set<std::string_view> cpp_exts = {
      ".h", ".hpp", ".c", ".cc", ".cp", ".C", ".cxx", ".cpp", ".c++"};
  auto pos = path.find_last_of("."sv);
  if (pos == std::string_view::npos) return false;
  std::string_view ext = path.substr(pos);
  return cpp_exts.count(ext);
}

namespace detail {
typedef struct {
  std::string_view dest;
  git_repository *repo;
} DumpTreePayload;

int fill_diff_set(const git_diff_delta *delta, float progress, void *payload) {
  auto diff_set =
      static_cast<std::unordered_set<sa::SimplifiedDiffDelta> *>(payload);
  if (is_cpp_file(delta->new_file.path)) {
    diff_set->insert(sa::SimplifiedDiffDelta{
        .OldPath = delta->old_file.path,
        .NewPath = delta->new_file.path,
        .Type = static_cast<sa::SimplifiedDiffDelta::DeltaType>(delta->status),
        .Similarity = delta->similarity});
  }
  return 0;
}

int dump_tree_entry(const char *root, const git_tree_entry *entry,
                    void *payload) {
  DumpTreePayload dump_payload = *static_cast<DumpTreePayload *>(payload);
  std::string_view dest = dump_payload.dest;
  git_repository *repo = dump_payload.repo;

  std::string entry_path = fs::path(root) / git_tree_entry_name(entry);
  std::string dest_path = fs::path(dest) / entry_path;

  git_object_t object_type = git_tree_entry_type(entry);
  if (object_type == GIT_OBJECT_TREE) {
    // create a directory for the tree entry
    if (!fs::exists(fs::path(dest_path)) &&
        mkdir(dest_path.c_str(), 0755) != 0) {
      spdlog::error("failed to create directory: {}, error reason: {}",
                    dest_path, strerror(errno));
      return -1;
    }

    // recursively dump the subtree
    git_tree *subtree = nullptr;
    int error = git_tree_lookup(&subtree, repo, git_tree_entry_id(entry));
    if (error < 0) {
      const git_error *e = git_error_last();
      spdlog::error("error to lookup subtree {}/{}: {}", error, e->klass,
                    e->message);
      return error;
    }

    dump_payload.dest = dest_path;
    size_t num_entries = git_tree_entrycount(subtree);
    tbb::parallel_for(
        tbb::blocked_range<size_t>(0, num_entries),
        [&](const tbb::blocked_range<size_t> &range) {
          for (auto i = range.begin(); i != range.end(); ++i) {
            const git_tree_entry *entry = git_tree_entry_byindex(subtree, i);
            detail::dump_tree_entry(dump_payload.dest.data(), entry, payload);
          }
        });
    git_tree_free(subtree);

    if (error < 0) {
      const git_error *e = git_error_last();
      spdlog::error("error to walk subtree {}/{}: {}", error, e->klass,
                    e->message);
      return error;
    }
  } else if (object_type == GIT_OBJECT_BLOB) {
    git_blob *blob = nullptr;
    int error = git_blob_lookup(&blob, repo, git_tree_entry_id(entry));
    if (error < 0) {
      const git_error *e = git_error_last();
      spdlog::error("error to lookup blob {}/{}: {}", error, e->klass,
                    e->message);
      return error;
    }

    const void *data = git_blob_rawcontent(blob);
    size_t size = git_blob_rawsize(blob);

    if (git_tree_entry_filemode(entry) == GIT_FILEMODE_LINK) {
      int res = symlink(static_cast<const char *>(data), dest_path.c_str());
      if (res != 0) {
        spdlog::error("symlink {} to {} failed", dest_path,
                      static_cast<const char *>(data));
        return 1;
      }
    } else {
      int output_fd = open(dest_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
                           S_IRUSR | S_IWUSR);
      if (output_fd == -1) {
        spdlog::error("failed to open output file: {}", dest_path);
        git_blob_free(blob);
        return -1;
      }

      ssize_t bytes_written = write(output_fd, data, size);
      if (bytes_written != static_cast<ssize_t>(size)) {
        spdlog::error("failed to write data to output file: {}", dest_path);
        close(output_fd);
        git_blob_free(blob);
        return -1;
      }

      close(output_fd);
      git_blob_free(blob);
    }
  } else {
    spdlog::warn("unexpected git object type found: {}", object_type);
    return 1;
  }

  return 0;
}
}  // namespace detail

std::unique_ptr<GitCommit> GitRepository::lookupCommit(
    std::string_view commit_hash) const {
  int err = -1;
  git_oid oid;
  git_commit *commit = nullptr;
  git_oid_fromstr(&oid, commit_hash.data());
  err = git_commit_lookup(&commit, repo, &oid);
  if (err < 0) {
    return std::make_unique<GitCommit>(nullptr);
  }
  return std::make_unique<GitCommit>(commit);
}

std::unique_ptr<GitCommit> GitRepository::lookupCommitByPrefix(
    std::string_view commit_hash) const {
  if (commit_hash.length() > GIT_OID_MAX_HEXSIZE) {
    return nullptr;
  }

  int err = -1;
  git_oid oid;
  git_commit *commit = nullptr;

  err = commit_hash.length() == GIT_OID_MAX_HEXSIZE
            ? git_oid_fromstr(&oid, commit_hash.data())
            : git_oid_fromstrp(&oid, commit_hash.data());
  if (err < 0) {
    const git_error *e = git_error_last();
    spdlog::debug(
        "fail to parse commit hash [{}] into git_oid, error {}/{}: {}",
        commit_hash, err, e->klass, e->message);
    return nullptr;
  }

  err = git_commit_lookup_prefix(&commit, repo, &oid, commit_hash.length());
  if (err < 0) return nullptr;

  return std::make_unique<GitCommit>(commit);
}

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
  err = git_diff_foreach(diff, detail::fill_diff_set, nullptr, nullptr, nullptr,
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

bool dump_tree_object_to(std::string_view dest, std::string_view commit_hash,
                         std::string_view repo_path) {
  fs::path dest_path = fs::path(dest);
  if (fs::exists(dest) && !fs::is_directory(dest)) {
    spdlog::error("dest {} is not a directory", dest);
    return false;
  }
  if (!fs::exists(dest)) {
    fs::create_directories(dest_path);
  }

  std::unique_ptr<GitRepository> repo_ptr =
      GitRepository::create(repo_path.data());
  if (!repo_ptr) {
    const git_error *e = git_error_last();
    spdlog::error("error {}: {}", e->klass, e->message);
    return false;
  }

  std::unique_ptr<GitCommit> commit_ptr = repo_ptr->lookupCommit(commit_hash);
  if (!commit_ptr) {
    const git_error *e = git_error_last();
    spdlog::error("error {}: {}", e->klass, e->message);
    return false;
  }

  std::unique_ptr<GitTree> tree_ptr = commit_ptr->tree();
  if (!tree_ptr) {
    const git_error *e = git_error_last();
    spdlog::error("error {}: {}", e->klass, e->message);
    return false;
  }

  size_t num_entries = git_tree_entrycount(tree_ptr->get());
  auto start = std::chrono::high_resolution_clock ::now();
  tbb::parallel_for(tbb::blocked_range<size_t>(0, num_entries),
                    [&](const tbb::blocked_range<size_t> &range) {
                      detail::DumpTreePayload payload = {dest, repo_ptr->get()};
                      for (auto i = range.begin(); i != range.end(); ++i) {
                        const git_tree_entry *entry =
                            git_tree_entry_byindex(tree_ptr->get(), i);
                        detail::dump_tree_entry("", entry, &payload);
                      }
                    });
  auto end = std::chrono::high_resolution_clock ::now();
  spdlog::debug(
      "it takes {}ms to dump commit tree object {} to {}",
      std::chrono::duration_cast<std::chrono::milliseconds>((end - start))
          .count(),
      commit_hash, dest);
  return true;
}

std::optional<std::string> full_commit_hash(const std::string &hash,
                                            const std::string &project_path) {
  std::optional<std::string> result = std::nullopt;

  std::unique_ptr<GitRepository> repo_ptr = nullptr;
  std::unique_ptr<GitCommit> commit_ptr = nullptr;
  git_oid full_oid;

  repo_ptr = GitRepository::create(project_path);
  if (!repo_ptr) return std::nullopt;

  commit_ptr = repo_ptr->lookupCommitByPrefix(hash);
  if (!commit_ptr) return std::nullopt;

  char full_hash[GIT_OID_MAX_HEXSIZE + 1];
  full_hash[GIT_OID_MAX_HEXSIZE] = '\0';
  git_oid_fmt(full_hash, git_commit_id(commit_ptr->get()));
  return std::string(full_hash);
}

std::optional<std::string> git_merge_base(const std::string &our,
                                          const std::string &their,
                                          const std::string &project_path) {
  std::string cmd =
      fmt::format("(cd {} && git merge-base {} {})", project_path, our, their);
  llvm::ErrorOr<std::string> resultOrErr = util::ExecCommand(cmd);
  if (!resultOrErr) return std::nullopt;
  std::string base = resultOrErr.get();
  if (base.length() != 40) {
    spdlog::warn(
        "base commit of our [{}] and their [{}] branches doesn't exist", our,
        their);
    return std::nullopt;
  }
  return base;
}
}  // namespace util
}  // namespace mergebot
