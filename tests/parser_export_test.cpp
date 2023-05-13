
#include <gtest/gtest.h>

#include "fcntl.h"
#include "mergebot/parser/parser.h"

namespace mergebot {
namespace sa {
TEST(ParserTest, ParserExportTest) {
  // clang-format off
  std::string source = R"(
#include <iostream>

template <typename T>
T max(const T& a, const T& b) {
    return a > b ? a : b;
}

int main() {
  int a = 1;
  int b = 2;
  int sum = a + b;
#ifndef NDEBUG
  std::cout << sum << "\n";
#endif
}
)";
  // clang-format on
  ts::Parser parser(ts::cpp::language());
  std::shared_ptr<ts::Tree> tree = parser.parse(source);
  int dot_file =
      open("tree.dot", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
  if (dot_file == -1) {
    perror("Failed to open DOT file");
    exit(1);
  }
  tree->exportDotGraph(dot_file);
}
}  // namespace sa
}  // namespace mergebot