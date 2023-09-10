//
// Created by whalien on 09/09/23.
//

#include <git2/errors.h>
#include <git2/global.h>
#include <git2/merge.h>
#include <gtest/gtest.h>

std::string threeWayMerge(const std::string& baseContent,
                          const std::string& leftContent,
                          const std::string& rightContent) {
  // Initialize libgit2
  git_libgit2_init();

  // Prepare git_merge_file_input structures for base, left, and right contents
  git_merge_file_input baseInput = GIT_MERGE_FILE_INPUT_INIT;
  git_merge_file_input leftInput = GIT_MERGE_FILE_INPUT_INIT;
  git_merge_file_input rightInput = GIT_MERGE_FILE_INPUT_INIT;

  baseInput.ptr = baseContent.c_str();
  baseInput.size = baseContent.size();

  leftInput.ptr = leftContent.c_str();
  leftInput.size = leftContent.size();

  rightInput.ptr = rightContent.c_str();
  rightInput.size = rightContent.size();

  // Prepare git_merge_file_result to hold the merge result
  git_merge_file_result mergeResult = {0};

  // Perform the 3-way merge
  //  git_merge_file_flag_t
  // File merging flags
  //
  // GIT_MERGE_FILE_DEFAULT
  // Defaults
  //
  // GIT_MERGE_FILE_STYLE_MERGE
  // Create standard conflicted merge files
  //
  // GIT_MERGE_FILE_STYLE_DIFF3
  // Create diff3-style files
  //
  // GIT_MERGE_FILE_SIMPLIFY_ALNUM
  // Condense non-alphanumeric regions for simplified diff file
  //
  // GIT_MERGE_FILE_IGNORE_WHITESPACE
  // Ignore all whitespace
  //
  // GIT_MERGE_FILE_IGNORE_WHITESPACE_CHANGE
  // Ignore changes in amount of whitespace
  //
  // GIT_MERGE_FILE_IGNORE_WHITESPACE_EOL
  // Ignore whitespace at end of line
  //
  // GIT_MERGE_FILE_DIFF_PATIENCE
  // Use the "patience diff" algorithm
  //
  // GIT_MERGE_FILE_DIFF_MINIMAL
  // Take extra time to find minimal diff
  //
  // GIT_MERGE_FILE_STYLE_ZDIFF3
  // Create zdiff3 ("zealous diff3")-style files
  //
  // GIT_MERGE_FILE_ACCEPT_CONFLICTS
  // Do not produce file conflicts when common regions have changed; keep
  // the conflict markers in the file and accept that as the merge result.
  int mergeStatus = git_merge_file(&mergeResult, &baseInput, &leftInput,
                                   &rightInput, nullptr);
  if (mergeStatus != 0) {
    std::cerr << "Merge failed: " << git_error_last()->message << std::endl;
    return "";
  }

  // Convert the merge result to a std::string
  std::string mergedContent(mergeResult.ptr, mergeResult.len);

  // Free the merge result
  git_merge_file_result_free(&mergeResult);

  // Shutdown libgit2
  git_libgit2_shutdown();

  return mergedContent;
}

TEST(Merge, Str) {
  std::string base = "This is the base content.\n";
  std::string left = "This is the left-side content.\n";
  std::string right = "This is the right-side content.\n";

  std::string mergedContent = threeWayMerge(base, left, right);

  std::cout << "Merged content:\n" << mergedContent << std::endl;
}