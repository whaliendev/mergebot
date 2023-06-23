//
// Created by whalien on 04/05/23.
//
#include "mergebot/utils/gitservice.h"

#include <gtest/gtest.h>

#include <filesystem>

#include "mergebot/core/model/SimplifiedDiffDelta.h"
#include "mergebot/filesystem.h"

namespace mergebot {
namespace sa {
TEST(GitServiceTest, ListCppDiffFilesTest) {
  std::string rocksdb_path = "/home/whalien/Desktop/demo";
  fs::path rocksdb = fs::path(rocksdb_path);
  if (!fs::exists(rocksdb) || !fs::is_directory(rocksdb)) {
    GTEST_SKIP();
  }
  std::unordered_set<SimplifiedDiffDelta> diff_set =
      mergebot::util::list_cpp_diff_files(
          rocksdb_path, "4a569d0ad3f8d35d5278604656e189f7d04f5044",
          "cb1b808ce5df1d442add84979b74ebdd7e4781ac");
  std::for_each(
      diff_set.begin(), diff_set.end(),
      [](SimplifiedDiffDelta const& sdd) { std::cout << sdd << "\n"; });
}

TEST(GitServiceTest, DumpTreeObjectTest) {
  std::string rocksdb_path = "/home/whalien/Desktop/rocksdb/.git";
  fs::path rocksdb = fs::path(rocksdb_path);
  if (!fs::exists(rocksdb) || !fs::is_directory(rocksdb)) {
    GTEST_SKIP();
  }

  std::string commit_hash = "28d1a0c6f59cfdc692a7274f8816b87af7a1d8cc";
  auto start = std::chrono::high_resolution_clock::now();
  mergebot::util::dump_tree_object_to("/tmp/rocksdb", commit_hash,
                                      rocksdb_path);
  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  spdlog::info("it takes {} ms to copy commit hash {}", duration.count(),
               commit_hash);
}
}  // namespace sa
}  // namespace mergebot