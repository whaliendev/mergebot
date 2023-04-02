//
// Created by whalien on 01/04/23.
//
#include "mergebot/utils/stringop.h"

#include <gtest/gtest.h>

TEST(StringOpTest, RemoveSpaces) {
  std::string data1 = "123\t4568910 abc\n890jk";
  std::string expected1 = "1234568910abc890jk";

  EXPECT_EQ(expected1, mergebot::util::removeSpaces(data1))
      << "fundamental test failed";

  std::string data2 = "abc\t\t\n\ropqrst ";
  std::string expected2 = "abcopqrst";
  std::string_view sv = data2;
  data2.clear();  // make the string invalid and release its memory
  EXPECT_NE(expected2, mergebot::util::removeSpaces(sv))
      << "data2 should be dead";
}
