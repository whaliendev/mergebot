//
// Created by whalien on 04/05/23.
//

#include "mergebot/utils/gitservice.h"

#include <fcntl.h>
#include <git2.h>
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_for.h>
#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <unordered_set>

#include "mergebot/core/model/SimplifiedDiffDelta.h"
#include "mergebot/core/model/enum/ConflictMark.h"
#include "mergebot/filesystem.h"
#include "mergebot/utils/fileio.h"

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
std::string corresponding_commit_hash(
    std::string_view hash, const std::unique_ptr<GitRepository> &repo_ptr) {
  git_object *obj = nullptr;
  git_oid oid;
  int error = git_oid_fromstr(&oid, hash.data());
  if (error != 0) {
    const git_error *e = git_error_last();
    spdlog::debug("fail to parse hash [{}] into oid, error {}/{}: {}", hash,
                  error, e->klass, e->message);
    return "";
  }

  error = git_object_lookup(&obj, repo_ptr->unwrap(), &oid, GIT_OBJECT_ANY);
  if (error != 0) {
    const git_error *e = git_error_last();
    spdlog::debug("error to parse hash [{}] into git object, error {}/{}: {}",
                  hash, error, e->klass, e->message);
    return "";
  }

  git_otype obj_type = git_object_type(obj);
  if (obj_type == GIT_OBJECT_TAG) {
    git_tag *tag = nullptr;
    error = git_tag_lookup(&tag, repo_ptr->unwrap(), &oid);
    if (error != 0) {
      const git_error *e = git_error_last();
      spdlog::debug("error to parse hash [{}] into git object, error {}/{}: {}",
                    hash, error, e->klass, e->message);
      git_object_free(obj);
      return "";
    }

    const git_oid *target_oid = git_tag_target_id(tag);
    char full_hash[GIT_OID_MAX_HEXSIZE + 1];
    full_hash[GIT_OID_MAX_HEXSIZE] = '\0';
    git_oid_fmt(full_hash, target_oid);

    git_tag_free(tag);
    git_object_free(obj);
    spdlog::debug("hash [{}] is a tag object, its commit hash is [{}]", hash,
                  full_hash);
    return std::string(full_hash);
  }

  git_object_free(obj);
  return std::string(hash);
}

typedef struct {
  std::string_view dest;
  git_repository *repo;
} DumpTreePayload;

int fill_diff_map(const git_diff_delta *delta, float progress, void *payload) {
  auto diff_map =
      static_cast<std::unordered_map<std::string, std::string> *>(payload);
  if (is_cpp_file(delta->new_file.path)) {
    diff_map->insert({delta->old_file.path, delta->new_file.path});
  }
  return 0;
}

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
    if (fs::exists(dest_path)) {
      return 1;
    }

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
        git_blob_free(blob);
        spdlog::error("symlink {} to {} failed", dest_path,
                      static_cast<const char *>(data));
        return 1;
      }
      git_blob_free(blob);
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
    spdlog::warn("unexpected git object type found: {}",
                 static_cast<int>(object_type));
    return 1;
  }

  return 0;
}

struct GetDiffHunkCBPayload {
  std::string old_file_path;
  std::string new_file_path;
  std::vector<size_t> old_offsets;
  std::vector<size_t> new_offsets;
  std::vector<MBDiffHunk> *diff_hunks;
};

int get_diff_hunk_cb(const git_diff_delta *delta, const git_diff_hunk *hunk,
                     void *payload) {
  GetDiffHunkCBPayload *cb_payload =
      static_cast<GetDiffHunkCBPayload *>(payload);
  std::string old_file_chunk =
      util::read_file_chunk(cb_payload->old_file_path, hunk->old_start,
                            hunk->old_lines, cb_payload->old_offsets);
  std::string new_file_chunk =
      util::read_file_chunk(cb_payload->new_file_path, hunk->new_start,
                            hunk->new_lines, cb_payload->new_offsets);

  using magic_enum::enum_name;
  using sa::ConflictMark;
  auto starts_with_conflict_mark = [](std::string_view str) {
    return util::starts_with(str, enum_name(ConflictMark::OURS)) ||
           util::starts_with(str, enum_name(ConflictMark::THEIRS)) ||
           util::starts_with(str, enum_name(ConflictMark::BASE)) ||
           util::starts_with(str, enum_name(ConflictMark::END));
  };

  auto only_one_line = [](std::string_view str) {
    return std::count(str.begin(), str.end(), '\n') == 1;
  };

  if (only_one_line(old_file_chunk) && only_one_line(new_file_chunk) &&
      starts_with_conflict_mark(old_file_chunk) &&
      starts_with_conflict_mark(new_file_chunk)) {
    return 0;
  }

  bool diff_in_spaces =
      util::diff_only_in_spaces(old_file_chunk, new_file_chunk);
  if (diff_in_spaces) {
    return 0;
  }

  MBDiffHunk diff_hunk = {.start = static_cast<size_t>(hunk->old_start),
                          .offset = static_cast<size_t>(hunk->old_lines),
                          .old_content = std::move(old_file_chunk),
                          .content = std::move(new_file_chunk)};

  cb_payload->diff_hunks->emplace_back(std::move(diff_hunk));
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
    return nullptr;
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

  size_t diff_num = 10;
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

  diff_num = git_diff_num_deltas(diff);
  diff_set.reserve(diff_num);

  // Note: very slow if the number of diff deltas is large
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

std::unordered_map<std::string, std::string> get_cpp_diff_mapping(
    std::string_view repo_path, std::string_view old_commit_str,
    std::string_view new_commit_str) {
  std::unordered_map<std::string, std::string> diff_map;

  size_t diff_num = 10;
  git_oid old_oid, new_oid;
  git_commit *old_commit = nullptr, *new_commit = nullptr;
  git_tree *old_tree = nullptr, *new_tree = nullptr;
  git_diff *diff = nullptr;
  //  git_diff_find_options options = GIT_DIFF_FIND_OPTIONS_INIT;
  //  options.flags = GIT_DIFF_FIND_RENAMES | GIT_DIFF_FIND_COPIES;
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

  diff_num = git_diff_num_deltas(diff);
  diff_map.reserve(diff_num);

  // Note: very slow if the number of diff deltas is large
  //  err = git_diff_find_similar(diff, &options);
  err = git_diff_foreach(diff, detail::fill_diff_map, nullptr, nullptr, nullptr,
                         &diff_map);
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
  return diff_map;
}

bool dump_tree_object_to(std::string_view dest, std::string_view hash,
                         std::string_view repo_path) {
  fs::path dest_path = fs::path(dest);
  if (fs::exists(dest) && !fs::is_directory(dest)) {
    spdlog::error("dest {} is not a directory", dest);
    return false;
  }
  fs::create_directories(dest_path);
  //  if (!fs::exists(dest)) {
  //    fs::create_directories(dest_path);
  //  }

  std::unique_ptr<GitRepository> repo_ptr =
      GitRepository::create(repo_path.data());
  if (!repo_ptr) {
    const git_error *e = git_error_last();
    spdlog::error("error {}: {}", e->klass, e->message);
    return false;
  }

  std::string commit_hash = detail::corresponding_commit_hash(hash, repo_ptr);

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

  size_t num_entries = git_tree_entrycount(tree_ptr->unwrap());
  spdlog::debug("there are {} tree entries of commit {} in {}", num_entries,
                commit_hash, repo_path);
  auto start = std::chrono::high_resolution_clock ::now();
  tbb::parallel_for(
      tbb::blocked_range<size_t>(0, num_entries),
      [&](const tbb::blocked_range<size_t> &range) {
        detail::DumpTreePayload payload = {dest, repo_ptr->unwrap()};
        for (auto i = range.begin(); i != range.end(); ++i) {
          const git_tree_entry *entry =
              git_tree_entry_byindex(tree_ptr->unwrap(), i);
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
  git_oid_fmt(full_hash, git_commit_id(commit_ptr->unwrap()));
  return std::string(full_hash);
}

std::optional<std::string> commit_hash_of_branch(
    const std::string &branch_name, const std::string &project_path) {
  std::unique_ptr<GitRepository> repo_ptr = nullptr;
  git_reference *ref;
  git_oid full_oid;

  repo_ptr = GitRepository::create(project_path);
  if (!repo_ptr) return std::nullopt;

  int res = git_branch_lookup(&ref, repo_ptr->unwrap(), branch_name.c_str(),
                              GIT_BRANCH_ALL);
  if (res != 0) return std::nullopt;

  std::string ref_name = git_reference_name(ref);
  int error =
      git_reference_name_to_id(&full_oid, repo_ptr->unwrap(), ref_name.c_str());
  if (error < 0) {
    git_reference_free(ref);
    return std::nullopt;
  }

  char full_hash[GIT_OID_MAX_HEXSIZE + 1];
  full_hash[GIT_OID_MAX_HEXSIZE] = '\0';
  git_oid_fmt(full_hash, &full_oid);
  git_reference_free(ref);
  return std::string(full_hash);
}

std::optional<std::string> git_merge_base(const std::string &our,
                                          const std::string &their,
                                          const std::string &project_path) {
  std::string cmd =
      fmt::format("(cd {} && git merge-base {} {})", project_path, our, their);
  llvm::ErrorOr<std::string> resultOrErr = utils::ExecCommand(cmd);
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

std::optional<std::string> commit_hash_of_rev(const std::string &revision,
                                              const std::string &project_path) {
  git_object *obj;

  std::unique_ptr<GitRepository> repo_ptr = GitRepository::create(project_path);
  int res = git_revparse_single(&obj, repo_ptr->unwrap(), revision.c_str());
  if (res != 0) {
    const git_error *err = git_error_last();
    spdlog::error(
        "fail to parse revision {}, error code: {}, error: {{ klass: {}, "
        "message: {} }}",
        revision, res, err->klass, err->message);
    return std::nullopt;
  }

  const git_oid *oid = git_object_id(obj);

  char full_hash[GIT_OID_MAX_HEXSIZE + 1];
  full_hash[GIT_OID_MAX_HEXSIZE] = '\0';
  git_oid_fmt(full_hash, oid);

  git_object_free(obj);

  std::string commit_hash =
      detail::corresponding_commit_hash(full_hash, repo_ptr);
  return commit_hash;
}

std::string git_merge_textual(const std::string &ours, const std::string &base,
                              const std::string &theirs,
                              const std::string &base_label,
                              const std::string &their_label,
                              const std::string &our_label) {
  git_merge_file_input our_input = GIT_MERGE_FILE_INPUT_INIT;
  our_input.ptr = ours.c_str();
  our_input.size = ours.size();

  git_merge_file_input base_input = GIT_MERGE_FILE_INPUT_INIT;
  base_input.ptr = base.c_str();
  base_input.size = base.size();

  git_merge_file_input their_input = GIT_MERGE_FILE_INPUT_INIT;
  their_input.ptr = theirs.c_str();
  their_input.size = theirs.size();

  git_merge_file_options opts = GIT_MERGE_FILE_OPTIONS_INIT;
  opts.ancestor_label = base_label.c_str();
  opts.our_label = our_label.c_str();
  opts.their_label = their_label.c_str();
  opts.flags = GIT_MERGE_FILE_STYLE_DIFF3 | GIT_MERGE_FILE_IGNORE_WHITESPACE |
               GIT_MERGE_FILE_IGNORE_WHITESPACE_CHANGE |
               GIT_MERGE_FILE_IGNORE_WHITESPACE_EOL |
               GIT_MERGE_FILE_DIFF_PATIENCE;

  git_merge_file_result result;
  // FIXME(hwa): squirrel bug, why ours and theirs are swapped?
  int error =
      git_merge_file(&result, &base_input, &our_input, &their_input, &opts);
  if (error < 0) {
    const git_error *e = git_error_last();
    spdlog::error("error to merge textual content {}/{}: {}", error, e->klass,
                  e->message);
    return "";
  }

  std::string merged_text(result.ptr, result.ptr + result.len);
  git_merge_file_result_free(&result);
  return merged_text;
}

std::vector<MBDiffHunk> get_git_diff_hunks(const std::string &old_path,
                                           const std::string &new_path) {
  if (!fs::exists(old_path) || !fs::is_regular_file(old_path)) {
    spdlog::error("old path {} doesn't exist or is not a regular file",
                  old_path);
    return {};
  }

  if (!fs::exists(new_path) || !fs::is_regular_file(new_path)) {
    spdlog::error("new path {} doesn't exist or is not a regular file",
                  new_path);
    return {};
  }

  std::vector<MBDiffHunk> diff_hunks;

  std::string old_file_content = util::file_get_content(old_path);
  std::string new_file_content = util::file_get_content(new_path);

  std::vector<size_t> old_offsets = util::compute_offsets(old_path);
  std::vector<size_t> new_offsets = util::compute_offsets(new_path);
  if (old_offsets.empty() || new_offsets.empty()) {
    spdlog::error("file {} or {} is empty", old_path, new_path);
    return {};
  }

  detail::GetDiffHunkCBPayload payload = {.old_file_path = old_path,
                                          .new_file_path = new_path,
                                          .old_offsets = std::move(old_offsets),
                                          .new_offsets = std::move(new_offsets),
                                          .diff_hunks = &diff_hunks};

  git_diff_options diff_opts = GIT_DIFF_OPTIONS_INIT;
  diff_opts.flags = GIT_DIFF_IGNORE_BLANK_LINES | GIT_DIFF_IGNORE_WHITESPACE |
                    GIT_DIFF_IGNORE_WHITESPACE_CHANGE |
                    GIT_DIFF_IGNORE_WHITESPACE_EOL;
  diff_opts.context_lines = 0;
  diff_opts.interhunk_lines = 3;

  int err = git_diff_buffers(
      old_file_content.data(), old_file_content.size(), nullptr,
      new_file_content.data(), new_file_content.size(), nullptr, &diff_opts,
      nullptr, nullptr, detail::get_diff_hunk_cb, nullptr, &payload);
  if (err < 0) {
    const git_error *e = git_error_last();
    spdlog::error("error to get diff hunks {}/{}: {}", err, e->klass,
                  e->message);
    return {};
  }

  return diff_hunks;
}
}  // namespace util
}  // namespace mergebot
