//
// Created by whalien on 06/09/23.
//
#include <gtest/gtest.h>
#include <re2/re2.h>

#include <iostream>
#include <string>
#include <string_view>

void ExtractNamespaceFields(std::string_view code) {
  std::string original_signature;
  std::string namespace_identifier;
  int n_offset = -1;

  // Adjusted regex pattern
  RE2 pattern(R"(((inline\s+)?namespace\s*([^\s{]*))\s*\{)");
  re2::StringPiece input(code.data(), code.size());

  if (RE2::PartialMatch(input, pattern, &original_signature, nullptr,
                        &namespace_identifier)) {
    if (!namespace_identifier.empty()) {
      n_offset = input.find(namespace_identifier) - 1;
    }
  }

  // Output results
  std::cout << "Original Signature: " << original_signature << "\n";
  std::cout << "Namespace Identifier: " << namespace_identifier << "\n";
  std::cout << "N Offset: " << n_offset << "\n";
}

TEST(Extract, ExtractNamespaceFields) {
  // Test four different namespace declarations
  ExtractNamespaceFields(R"(namespace aa {
  static const int a = 3;
  void dosomething();
})");
  std::cout << "-------------------"
            << "\n";

  ExtractNamespaceFields(R"(inline namespace bb {
  static const int a = 3;
  void dosomething();
})");
  std::cout << "-------------------"
            << "\n";

  ExtractNamespaceFields(R"(namespace {
  static const int a = 3;
  void dosomething();
})");
  std::cout << "-------------------"
            << "\n";

  ExtractNamespaceFields(R"(namespace A::B::C {
  static const int a = 3;
  void dosomething();
})");
}

TEST(Match, HeaderGuard) {
  // 示例C++代码，包含header guard
  std::string code = R"(
#ifndef MY_HEADER_GUARD
#define MY_HEADER_GUARD
// 其他C++代码
constexpr int kN = 3;
#endif // MY_HEADER_GUARD
)";

  // 正则表达式，用于匹配和提取header guard
  std::string pattern =
      R"(#ifndef\s+(\w+)\s*\n\s*#define\s+\1\s*\n.*#endif\s*(//.*)?)";

  RE2 re(pattern);

  // 用于存储匹配和提取的结果
  std::string ifndef_identifier;
  std::string endif_comment;

  // 执行匹配和提取
  if (RE2::FullMatch(code, re, &ifndef_identifier, &endif_comment)) {
    std::cout << "匹配成功！\n";
    std::cout << "Identifier: " << ifndef_identifier << "\n";
    std::cout << "Endif Comment: " << endif_comment << "\n";
  } else {
    std::cout << "匹配失败。\n";
  }
}
