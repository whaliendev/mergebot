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
  std::string frameworks_av_path;

  void SetUp() override {
    rocksdb_path = "/home/whalien/Desktop/rocksdb/.git";
    frameworks_av_path = "/home/whalien/Desktop/frameworks_av/.git";
    fs::path rocksdb(rocksdb_path);
    if (!fs::exists(rocksdb) || !fs::is_directory(rocksdb)) {
      GTEST_SKIP();
    }

    fs::path frameworks_av(frameworks_av_path);
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

// TEST_F(RepoBasedTest, FullCommitHashTest) {
//   // test resolve (full hash)
//   std::string validHash = "8ea21a778bb90d2f8c352b732c13ab64484eb386";
//   auto commit_hash = util::full_commit_hash(validHash, rocksdb_path);
//   EXPECT_TRUE(commit_hash.has_value()) << "commit hash is legal but failed";
//   EXPECT_EQ(commit_hash.value().size(), GIT_OID_MAX_HEXSIZE)
//       << "completeness of `full_commit_hash` test failed";
//
//   // test fail to resolve (full hash but an illegal one)
//   std::string illegalHash = "8ea21a778bb90d2f8c352b732c13ab64484euvwa";
//   commit_hash = util::full_commit_hash(illegalHash, rocksdb_path);
//   EXPECT_FALSE(commit_hash.has_value())
//       << "illegal commit hash should return std::nullopt";
//
//   // test completeness (short form)
//   std::string abbreviatedHash = "12966ec1bb22fecf9";
//   commit_hash = util::full_commit_hash(abbreviatedHash, rocksdb_path);
//   EXPECT_TRUE(commit_hash.has_value())
//       << "abbreviated should be parsed correctly";
//   EXPECT_EQ(commit_hash.value().size(), GIT_OID_MAX_HEXSIZE)
//       << "full_commit_hash should be able to parse short commit hash";
//
//   // test uniqueness
//   std::string nonUniqueHash = "832";
//   commit_hash = util::full_commit_hash(nonUniqueHash, rocksdb_path);
//   EXPECT_FALSE(commit_hash.has_value())
//       << "non unique commit hash should return std::nullopt";
// }

TEST_F(RepoBasedTest, BranchResolution) {
  std::string localBranch = "master";
  auto commit_hash =
      util::commit_hash_of_branch(localBranch, frameworks_av_path);
  EXPECT_TRUE(commit_hash.has_value())
      << "valid local branch should resolve successfully";
  EXPECT_EQ(commit_hash.value().size(), GIT_OID_MAX_HEXSIZE)
      << "valid local branch should resolve to a commit hash of length 40";

  std::string remoteBranch = "aosp/android10-qpr1-b-s1-release";
  commit_hash = util::commit_hash_of_branch(remoteBranch, frameworks_av_path);
  EXPECT_TRUE(commit_hash.has_value())
      << "valid remote branch should resolve successfully";
  EXPECT_EQ(commit_hash.value().size(), GIT_OID_MAX_HEXSIZE)
      << "valid remote branch should resolve to a commit hash of length 40";

  std::string invalidBranch = "android10-qprk-b-s1-release";
  commit_hash = util::commit_hash_of_branch(invalidBranch, frameworks_av_path);
  EXPECT_FALSE(commit_hash.has_value()) << "fake branch should fail to resolve";
}

TEST_F(RepoBasedTest, RevResolution) {
  // test resolve (full hash)
  std::string validHash = "8ea21a778bb90d2f8c352b732c13ab64484eb386";
  auto commit_hash = util::commit_hash_of_rev(validHash, rocksdb_path);
  EXPECT_TRUE(commit_hash.has_value()) << "commit hash is legal but failed";
  EXPECT_EQ(commit_hash.value().size(), GIT_OID_MAX_HEXSIZE)
      << "completeness of `full_commit_hash` test failed";
  EXPECT_EQ(commit_hash.value(), "8ea21a778bb90d2f8c352b732c13ab64484eb386")
      << "full commit hash should resolve successfully";

  // test fail to resolve (full hash but an illegal one)
  std::string illegalHash = "8ea21a778bb90d2f8c352b732c13ab64484euvwa";
  commit_hash = util::commit_hash_of_rev(illegalHash, rocksdb_path);
  EXPECT_FALSE(commit_hash.has_value())
      << "illegal commit hash should return std::nullopt";

  // test completeness (short form)
  std::string abbreviatedHash = "12966ec1bb22fecf9";
  commit_hash = util::commit_hash_of_rev(abbreviatedHash, rocksdb_path);
  EXPECT_TRUE(commit_hash.has_value())
      << "abbreviated should be parsed correctly";
  EXPECT_EQ(commit_hash.value().size(), GIT_OID_MAX_HEXSIZE)
      << "full_commit_hash should be able to parse short commit hash";
  EXPECT_EQ(commit_hash.value(), "12966ec1bb22fecf9f43099b13626953d5d3e661")
      << "valid short hash should resolve successfully";

  // test uniqueness
  std::string nonUniqueHash = "832";
  commit_hash = util::commit_hash_of_rev(nonUniqueHash, rocksdb_path);
  EXPECT_FALSE(commit_hash.has_value())
      << "non unique commit hash should return std::nullopt";

  // local branch test
  std::string localBranch = "master";
  commit_hash = util::commit_hash_of_rev(localBranch, frameworks_av_path);
  EXPECT_TRUE(commit_hash.has_value())
      << "valid local branch should resolve successfully";
  EXPECT_EQ(commit_hash.value().size(), GIT_OID_MAX_HEXSIZE)
      << "valid local branch should resolve to a commit hash of length 40";

  // remote branch test
  std::string remoteBranch = "aosp/android10-qpr1-b-s1-release";
  commit_hash = util::commit_hash_of_rev(remoteBranch, frameworks_av_path);
  EXPECT_TRUE(commit_hash.has_value())
      << "valid remote branch should resolve successfully";
  EXPECT_EQ(commit_hash.value().size(), GIT_OID_MAX_HEXSIZE)
      << "valid remote branch should resolve to a commit hash of length 40";
  EXPECT_EQ(commit_hash.value(), "0666991b1a4d6f04983fd89d8321a02cfdc523e4")
      << "valid branch name should resolve successfully";

  // remote branch test
  std::string abbrevRemoteBranch = "android10-mainline-resolv-release";
  commit_hash =
      util::commit_hash_of_rev(abbrevRemoteBranch, frameworks_av_path);
  EXPECT_FALSE(commit_hash.has_value())
      << "non local branch hash should return std::nullopt";

  // invalid branch test
  std::string invalidBranch = "android10-qprk-b-s1-release";
  commit_hash = util::commit_hash_of_rev(invalidBranch, frameworks_av_path);
  EXPECT_FALSE(commit_hash.has_value()) << "fake branch should fail to resolve";

  // valid tag
  std::string validTag = "aml_adb_331011050";
  commit_hash = util::commit_hash_of_rev(validTag, frameworks_av_path);
  EXPECT_TRUE(commit_hash.has_value())
      << "valid tag should resolve successfully";
  EXPECT_EQ(commit_hash.value().size(), GIT_OID_MAX_HEXSIZE)
      << "valid tag should resolve to a hash of length 40";
  EXPECT_EQ(commit_hash.value(), "a84dfe6a4bd926988f3a3befbb52f8b38c7fb35b")
      << "valid tag name should resolve successfully";

  std::string invalidTag = "aml_adb_33101105u";
  commit_hash = util::commit_hash_of_rev(invalidTag, frameworks_av_path);
  EXPECT_FALSE(commit_hash.has_value()) << "fake tag should fail to resolve";
}

}  // namespace sa
}  // namespace mergebot