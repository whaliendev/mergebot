//
// Created by whalien on 02/04/23.
//
#include <gtest/gtest.h>

#include <magic_enum.hpp>

#include "mergebot/core/model/enum/ConflictMark.h"
#include "mergebot/core/sa_utility.h"

TEST(SADetailsTest, ExtractCodeFromConflictRangeTest) {
  // clang-format off
  std::string TwoSideCF = R"(
<<<<<<< /some/dummy/filepath
this is a line of code, of course our side's code
and this is another line of code
=======
hello, world from whu
I'm in sa_details_test
>>>>>>> /another/dummy/filepath)";
  std::pair<std::string_view, std::string_view> ExpectedTwoSideCodes = {
      std::string_view {
R"(this is a line of code, of course our side's code
and this is another line of code
)"},
      std::string_view {
R"(hello, world from whu
I'm in sa_details_test
)"}
  };
  // clang-format on
  using namespace mergebot::sa;
  std::string_view OurCode = extractCodeFromConflictRange(
      TwoSideCF, magic_enum::enum_name(ConflictMark::OURS),
      magic_enum::enum_name(ConflictMark::THEIRS));
  std::string_view TheirCode = extractCodeFromConflictRange(
      TwoSideCF, magic_enum::enum_name(ConflictMark::THEIRS),
      magic_enum::enum_name(ConflictMark::END));
  EXPECT_EQ(ExpectedTwoSideCodes, std::make_pair(OurCode, TheirCode));

  // clang-format off
  std::string CppCheckCF =
R"(<<<<<<< /home/whalien/Desktop/projects/cppcheck/conflicts/7d2c26bd25a1d5ef030d30e2985e0f5ddb9ddf6d/gui/mainwindow.cpp/conflict.cpp
            for (int i = 0;i < languages.size();i++)
||||||| /home/whalien/Desktop/projects/cppcheck/conflicts/7d2c26bd25a1d5ef030d30e2985e0f5ddb9ddf6d/gui/mainwindow.cpp/base.cpp
            for (int i=0;i<languages.size();i++)
=======
            for (int i = 0; i < languages.size(); i++)
>>>>>>> /home/whalien/Desktop/projects/cppcheck/conflicts/7d2c26bd25a1d5ef030d30e2985e0f5ddb9ddf6d/gui/mainwindow.cpp/theirs.cpp>>>>>>>)";
  std::pair<std::string_view, std::string_view> ExpectedCppCheckCode = {
      std::string_view {
R"(            for (int i = 0;i < languages.size();i++)
)"},
      std::string_view {
R"(            for (int i = 0; i < languages.size(); i++)
)"}
  };
  // clang-format on
  using namespace mergebot::sa;
  OurCode = extractCodeFromConflictRange(
      CppCheckCF, magic_enum::enum_name(ConflictMark::OURS),
      magic_enum::enum_name(ConflictMark::BASE));
  TheirCode = extractCodeFromConflictRange(
      CppCheckCF, magic_enum::enum_name(ConflictMark::THEIRS),
      magic_enum::enum_name(ConflictMark::END));
  EXPECT_EQ(ExpectedCppCheckCode, std::make_pair(OurCode, TheirCode));
}

TEST(SADetailsTest, ExtractCodeFromConflictRangeDeathTest) {
  // clang-format off
  std::string OurSideDeathCF = R"(
this is a line of code, of course our side's code
and this is another line of code
=======
hello, world from whu
I'm in sa_details_test
>>>>>>> /another/dummy/filepath)";
  // clang-format on
  using namespace mergebot::sa;
  //  std::string_view OurCode = _details::extractCodeFromConflictRange(
  //      OurSideDeathCF, magic_enum::enum_name(ConflictMark::OURS),
  //      magic_enum::enum_name(ConflictMark::THEIRS));
  //  std::string_view TheirCode = _details::extractCodeFromConflictRange(
  //      OurSideDeathCF, magic_enum::enum_name(ConflictMark::THEIRS),
  //      magic_enum::enum_name(ConflictMark::END));
  ASSERT_DEATH(
      {
        extractCodeFromConflictRange(
            OurSideDeathCF, magic_enum::enum_name(ConflictMark::OURS),
            magic_enum::enum_name(ConflictMark::THEIRS));
      },
      "illegal conflict range, start marker line is in bad format");
}
