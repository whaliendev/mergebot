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
class RepoBasedTest : public ::testing::Test {
 protected:
  std::string rocksdb_path;

  void SetUp() override {
    rocksdb_path = "/home/whalien/Desktop/rocksdb/.git";
    fs::path rocksdb(rocksdb_path);
    if (!fs::exists(rocksdb) || !fs::is_directory(rocksdb)) {
      GTEST_SKIP();
    }
  }
};

TEST_F(RepoBasedTest, ListCppDiffFilesTest) {
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

TEST_F(RepoBasedTest, DumpTreeObjectTest) {
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

TEST_F(RepoBasedTest, FullCommitHashTest) {
  // test resolve (full hash)
  std::string validHash = "8ea21a778bb90d2f8c352b732c13ab64484eb386";
  auto commit_hash = util::full_commit_hash(validHash, rocksdb_path);
  EXPECT_TRUE(commit_hash.has_value()) << "commit hash is legal but failed";
  EXPECT_EQ(commit_hash.value().size(), GIT_OID_MAX_HEXSIZE)
      << "completeness of `full_commit_hash` test failed";

  // test fail to resolve (full hash but an illegal one)
  std::string illegalHash = "8ea21a778bb90d2f8c352b732c13ab64484euvwa";
  commit_hash = util::full_commit_hash(illegalHash, rocksdb_path);
  EXPECT_FALSE(commit_hash.has_value())
      << "illegal commit hash should return std::nullopt";

  // test completeness (short form)
  std::string abbreviatedHash = "12966ec1bb22fecf9";
  commit_hash = util::full_commit_hash(abbreviatedHash, rocksdb_path);
  EXPECT_TRUE(commit_hash.has_value())
      << "abbreviated should be parsed correctly";
  EXPECT_EQ(commit_hash.value().size(), GIT_OID_MAX_HEXSIZE)
      << "full_commit_hash should be able to parse short commit hash";

  // test uniqueness
  std::string nonUniqueHash = "832";
  commit_hash = util::full_commit_hash(nonUniqueHash, rocksdb_path);
  EXPECT_FALSE(commit_hash.has_value())
      << "non unique commit hash should return std::nullopt";
}

}  // namespace sa
}  // namespace mergebot