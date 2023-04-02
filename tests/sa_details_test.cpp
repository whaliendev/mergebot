//
// Created by whalien on 02/04/23.
//
#include <gtest/gtest.h>

#include <magic_enum.hpp>

#include "mergebot/core/model/ConflictMark.h"
#include "mergebot/magic_enum_customization.h"

namespace mergebot {
namespace sa {
namespace _details {
std::string_view extractCodeFromConflictRange(std::string_view Source,
                                              std::string_view StartMark,
                                              std::string_view EndMarker);
}
}  // namespace sa
}  // namespace mergebot

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
and this is another line of code)"},
      std::string_view {
R"(hello, world from whu
I'm in sa_details_test)"
      }
  };
  // clang-format on
  using namespace mergebot::sa;
  std::string_view OurCode = _details::extractCodeFromConflictRange(
      TwoSideCF, magic_enum::enum_name(ConflictMark::OURS),
      magic_enum::enum_name(ConflictMark::THEIRS));
  std::string_view TheirCode = _details::extractCodeFromConflictRange(
      TwoSideCF, magic_enum::enum_name(ConflictMark::THEIRS),
      magic_enum::enum_name(ConflictMark::END));
  EXPECT_EQ(ExpectedTwoSideCodes, std::make_pair(OurCode, TheirCode));
}
