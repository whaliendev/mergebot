//
// Created by whalien on 06/09/23.
//
#include <gtest/gtest.h>
#include <re2/re2.h>

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "mergebot/parser/utils.h"

void ExtractNamespaceFields(std::string_view code) {
  std::string DisplayName;
  std::string OriginalSignature;
  std::string Inline;
  int NOffset = 0;
  RE2 pattern(R"(((inline\s+)?namespace\s*([^\s{]*)\s*)\{)");
  re2::StringPiece input(code);
  if (RE2::PartialMatch(input, pattern, &OriginalSignature, &Inline,
                        &DisplayName)) {
    if (!DisplayName.empty()) {
      NOffset = input.find(DisplayName);
      size_t QualifiedOffset = DisplayName.rfind("::");
      if (QualifiedOffset != std::string::npos) {
        NOffset += (QualifiedOffset + 2);  // 2 for "::"
      }
    }
  } else {
    assert(false && "it seems that Node is not a namespace node");
  }

  // Output results
  std::cout << "Original Signature: " << OriginalSignature << "\n";
  std::cout << "Namespace Identifier: " << DisplayName << "\n";
  std::cout << "N Offset: " << NOffset << "\n";
  std::cout << "inline: " << Inline << "\n";
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

  ExtractNamespaceFields(
      "namespace rocksdb {\n"
      "\n"
      "std::unique_ptr<ThreadLocalPtr::StaticMeta> "
      "ThreadLocalPtr::StaticMeta::inst_;\n"
      "port::Mutex ThreadLocalPtr::StaticMeta::mutex_;\n"
      "#if !defined(OS_MACOSX)\n"
      "__thread ThreadLocalPtr::ThreadData* ThreadLocalPtr::StaticMeta::tls_ = "
      "nullptr;\n"
      "#endif");
}

// TEST(Match, HeaderGuard) {
//   // 示例C++代码，包含header guard
//   std::string code = R"(
// #ifndef MY_HEADER_GUARD
// #define MY_HEADER_GUARD
//// 其他C++代码
// constexpr int kN = 3;
// #endif // MY_HEADER_GUARD
//)";
//
//   // 正则表达式，用于匹配和提取header guard
//   std::string pattern =
//       R"(#ifndef\s+(\w+)\s*\n\s*#define\s+\1\s*\n.*#endif\s*(//.*)?)";
//
//   RE2 re(pattern);
//
//   // 用于存储匹配和提取的结果
//   std::string ifndef_identifier;
//   std::string endif_comment;
//
//   // 执行匹配和提取
//   if (RE2::FullMatch(code, re, &ifndef_identifier, &endif_comment)) {
//     std::cout << "匹配成功！\n";
//     std::cout << "Identifier: " << ifndef_identifier << "\n";
//     std::cout << "Endif Comment: " << endif_comment << "\n";
//   } else {
//     std::cout << "匹配失败。\n";
//   }
// }

void ExtractEnumInfo(const std::string& code) {
  const std::string pattern =
      R"(((enum\s*(class|struct)?)((\s*\[\[[^\]]+\]\])*)?\s*([a-zA-Z_][a-zA-Z0-9_:]*)?\s*(:\s*[a-zA-Z_][a-zA-Z0-9_]*)?)\s*\{)";
  re2::RE2 re(pattern);

  std::string EnumKey, Attrs, EnumName, EnumBase, OriginalSignature;
  re2::StringPiece input(code);
  re2::StringPiece enum_name_pieces;

  if (re2::RE2::PartialMatch(input, re, &OriginalSignature, &EnumKey, nullptr,
                             &Attrs, nullptr, &enum_name_pieces, &EnumBase)) {
    EnumName = enum_name_pieces;
    std::cout << "Original Signature: " << OriginalSignature << "\n";
    std::cout << "Enum Key: " << EnumKey << "\n";
    std::cout << "Attrs: " << Attrs << "\n";
    std::cout << "Enum Name: " << EnumName << "\n";

    // 获取最后一个层级名称的位置
    size_t last_colon = enum_name_pieces.rfind("::");
    size_t offset_from_start =
        enum_name_pieces.empty() ? 0
        : last_colon == std::string::npos
            ? enum_name_pieces.data() - input.data()
            : enum_name_pieces.data() - input.data() + last_colon + 2;
    std::cout << "Offset: " << offset_from_start << "\n";
  }
}

TEST(Extract, ExtractEnumInfo) {
  // Test codes
  std::string code1 = "enum class [[attr1]] [[attr2]] Week { MON, TUE, WED };";
  std::string code2 = "enum [[deprecated]] A::B::C { };";
  std::string code3 = "enum struct : base { };";
  std::string code4 = "enum K : base { };";
  std::string code5 = "enum struct [[deprecated]] A::B::C: uint16_t { };";
  std::string code6 = "enum [[inline]] A { };";

  std::cout << "Code 1:"
            << "\n";
  ExtractEnumInfo(code1);
  std::cout << "\n";

  std::cout << "Code 2:"
            << "\n";
  ExtractEnumInfo(code2);
  std::cout << "\n";

  std::cout << "Code 3:"
            << "\n";
  ExtractEnumInfo(code3);
  std::cout << "\n";

  std::cout << "Code 4:"
            << "\n";
  ExtractEnumInfo(code4);
  std::cout << "\n";

  std::cout << "Code 5:"
            << "\n";
  ExtractEnumInfo(code5);
  std::cout << "\n";

  std::cout << "Code 6:"
            << "\n";
  ExtractEnumInfo(code6);
  std::cout << "\n";
}

TEST(Extract, ExtractClassInfo) {
  using namespace mergebot;
  std::string code1 = R"(template <typename T, typename U>
class [[deprecated]] [[attr1]] KKK final : public UUU { ... })";
  ts::ClassInfo expected1 = {
      .OriginalSignature =
          "template <typename T, typename U>\n"
          "class [[deprecated]] [[attr1]] KKK final : public UUU ",
      .TemplateParameterList = "template <typename T, typename U>",
      .ClassKey = "class",
      .Attrs = "[[deprecated]] [[attr1]]",
      .ClassName = "KKK",
      .IsFinal = true,
      .BaseClause = ": public UUU ",
      .LineOffset = 1,
      .ColOffset = 31};
  ASSERT_EQ(expected1, ts::extractClassInfo(code1));

  std::string code2 = R"(struct [[deprecated]] A::B::C { })";
  ts::ClassInfo expected2 = {
      .OriginalSignature = "struct [[deprecated]] A::B::C ",
      .TemplateParameterList = "",
      .ClassKey = "struct",
      .Attrs = "[[deprecated]]",
      .ClassName = "A::B::C",
      .IsFinal = false,
      .BaseClause = "",
      .LineOffset = 0,
      .ColOffset = 28};
  ASSERT_EQ(expected2, ts::extractClassInfo(code2));

  std::string code3 = R"(union : base { })";
  ts::ClassInfo expected3 = {.OriginalSignature = "union : base ",
                             .TemplateParameterList = "",
                             .ClassKey = "union",
                             .Attrs = "",
                             .ClassName = "",
                             .IsFinal = false,
                             .BaseClause = ": base ",
                             .LineOffset = 0,
                             .ColOffset = 0};
  ASSERT_EQ(expected3, ts::extractClassInfo(code3));

  std::string code4 = R"(class K : base { })";
  ts::ClassInfo expected4 = {.OriginalSignature = "class K : base ",
                             .TemplateParameterList = "",
                             .ClassKey = "class",
                             .Attrs = "",
                             .ClassName = "K",
                             .IsFinal = false,
                             .BaseClause = ": base ",
                             .LineOffset = 0,
                             .ColOffset = 6};
  ASSERT_EQ(expected4, ts::extractClassInfo(code4));

  std::string code5 = R"(struct [[deprecated]] A::B::C: base { })";
  ts::ClassInfo expected5 = {
      .OriginalSignature = "struct [[deprecated]] A::B::C: base ",
      .TemplateParameterList = "",
      .ClassKey = "struct",
      .Attrs = "[[deprecated]]",
      .ClassName = "A::B::C",
      .IsFinal = false,
      .BaseClause = ": base ",
      .LineOffset = 0,
      .ColOffset = 28};
  ASSERT_EQ(expected5, ts::extractClassInfo(code5));

  std::string code6 = R"(union A { })";
  ts::ClassInfo expected6 = {.OriginalSignature = "union A ",
                             .TemplateParameterList = "",
                             .ClassKey = "union",
                             .Attrs = "",
                             .ClassName = "A",
                             .IsFinal = false,
                             .BaseClause = "",
                             .LineOffset = 0,
                             .ColOffset = 6};
  ASSERT_EQ(expected6, ts::extractClassInfo(code6));

  std::string code7 = R"(template <typename T>
class A { })";
  ts::ClassInfo expected7 = {
      .OriginalSignature = "template <typename T>\nclass A ",
      .TemplateParameterList = "template <typename T>",
      .ClassKey = "class",
      .Attrs = "",
      .ClassName = "A",
      .IsFinal = false,
      .BaseClause = "",
      .LineOffset = 1,
      .ColOffset = 6};
  ASSERT_EQ(expected7, ts::extractClassInfo(code7));

  std::string code8 = R"(template <typename T>
class
A { })";
  ts::ClassInfo expected8 = {
      .OriginalSignature = "template <typename T>\nclass\nA ",
      .TemplateParameterList = "template <typename T>",
      .ClassKey = "class",
      .Attrs = "",
      .ClassName = "A",
      .IsFinal = false,
      .BaseClause = "",
      .LineOffset = 2,
      .ColOffset = 0};
  ASSERT_EQ(expected8, ts::extractClassInfo(code8));

  std::string code9 = R"(template <typename T>
class [[deprecated]]
A { })";
  ts::ClassInfo expected9 = {
      .OriginalSignature = "template <typename T>\nclass [[deprecated]]\nA ",
      .TemplateParameterList = "template <typename T>",
      .ClassKey = "class",
      .Attrs = "[[deprecated]]",
      .ClassName = "A",
      .IsFinal = false,
      .BaseClause = "",
      .LineOffset = 2,
      .ColOffset = 0};
  ASSERT_EQ(expected9, ts::extractClassInfo(code9));
}

TEST(Extract, FuncSpecialMember) {
  using mergebot::ts::extractFuncSpecialMemberInfo;
  using mergebot::ts::FuncSpecialMemberInfo;
  using DefinitionType = mergebot::sa::FuncSpecialMemberNode::DefinitionType;
  // Test Case 1: Template with attributes and initializer list
  std::string code1 =
      R"(template <typename T> [[nodiscard]] MyClass::MyClass(const T& t) : member(t) { /* body */ })";
  FuncSpecialMemberInfo expected1 = {
      .DefType = DefinitionType ::Plain,
      .OriginalSignature =
          "template <typename T> [[nodiscard]] MyClass::MyClass(const T& t) : "
          "member(t) ",
      .TemplateParameterList = "template <typename T>",
      .Attrs = "[[nodiscard]]",
      .BeforeFuncName = "",
      .FuncName = "MyClass::MyClass",
      .ParameterList = {"const T& t"},
      .InitList = {"member(t)"},
      .LineOffset = 0,
      .ColOffset = 45};
  EXPECT_EQ(expected1, extractFuncSpecialMemberInfo(code1));

  // Test Case 2: Deleted function
  std::string code2 = "MyClass::MyClass() = delete;";
  FuncSpecialMemberInfo expected2 = {.DefType = DefinitionType ::Deleted,
                                     .OriginalSignature = "MyClass::MyClass() ",
                                     .TemplateParameterList = "",
                                     .Attrs = "",
                                     .BeforeFuncName = "",
                                     .FuncName = "MyClass::MyClass",
                                     .ParameterList = {},
                                     .InitList = {},
                                     .LineOffset = 0,
                                     .ColOffset = 9};
  EXPECT_EQ(expected2, extractFuncSpecialMemberInfo(code2));

  // Test Case 3: Defaulted function
  std::string code3 = "MyClass::MyClass() = default;";
  FuncSpecialMemberInfo expected3 = {.DefType = DefinitionType ::Defaulted,
                                     .OriginalSignature = "MyClass::MyClass() ",
                                     .TemplateParameterList = "",
                                     .Attrs = "",
                                     .BeforeFuncName = "",
                                     .FuncName = "MyClass::MyClass",
                                     .ParameterList = {},
                                     .InitList = {},
                                     .LineOffset = 0,
                                     .ColOffset = 9};
  EXPECT_EQ(expected3, extractFuncSpecialMemberInfo(code3));

  // Test Case 4: Multiple parameters and multiple initializer list
  std::string code4 = "MyClass::MyClass(int a, int b) : a(a), b(b) {}";
  FuncSpecialMemberInfo expected4 = {
      .DefType = DefinitionType ::Plain,
      .OriginalSignature = "MyClass::MyClass(int a, int b) : a(a), b(b) ",
      .TemplateParameterList = "",
      .Attrs = "",
      .BeforeFuncName = "",
      .FuncName = "MyClass::MyClass",
      .ParameterList = {"int a", "int b"},
      .InitList = {"a(a)", "b(b)"},
      .LineOffset = 0,
      .ColOffset = 9};
  EXPECT_EQ(expected4, extractFuncSpecialMemberInfo(code4));

  // Test Case 5: No attributes, no template, no initializer list
  std::string code5 = "MyClass::MyClass(int a) {}";
  FuncSpecialMemberInfo expected5 = {
      .DefType = DefinitionType ::Plain,
      .OriginalSignature = "MyClass::MyClass(int a) ",
      .TemplateParameterList = "",
      .Attrs = "",
      .BeforeFuncName = "",
      .FuncName = "MyClass::MyClass",
      .ParameterList = {"int a"},
      .InitList = {},
      .LineOffset = 0,
      .ColOffset = 9};
  EXPECT_EQ(expected5, extractFuncSpecialMemberInfo(code5));

  // Test Case 6: Multi-line code
  std::string code6 = R"(MyClass::MyClass(
    int a,
    int b
  ) : a(a),
      b(b)
  {
    // body
  })";
  FuncSpecialMemberInfo expected6 = {.DefType = DefinitionType ::Plain,
                                     .OriginalSignature =
                                         "MyClass::MyClass(\n"
                                         "    int a,\n"
                                         "    int b\n"
                                         "  ) : a(a),\n"
                                         "      b(b)\n"
                                         "  ",
                                     .TemplateParameterList = "",
                                     .Attrs = "",
                                     .BeforeFuncName = "",
                                     .FuncName = "MyClass::MyClass",
                                     .ParameterList = {"int a", "int b"},
                                     .InitList = {"a(a)", "b(b)"},
                                     .LineOffset = 0,
                                     .ColOffset = 9};
  ASSERT_EQ(expected6, extractFuncSpecialMemberInfo(code6));

  // Test Case 8: Invalid code (missing opening brace)
  EXPECT_DEATH(
      {
        std::string code8 = "MyClass::MyClass(int a, int b) : a(a), b(b) ";
        extractFuncSpecialMemberInfo(code8);
      },
      "it seems that the code is not a special member function");

  // Test Case 9: Code with extra spaces and tabs
  std::string code9 =
      "  MyClass::MyClass(   int a   , int   b ) :   a(a) , b(b) noexcept {}";
  FuncSpecialMemberInfo expected9 = {
      .DefType = DefinitionType ::Plain,
      .OriginalSignature =
          "  MyClass::MyClass(   int a   , int   b ) :   a(a) , b(b) noexcept ",
      .TemplateParameterList = "",
      .Attrs = "",
      .BeforeFuncName = "",
      .FuncName = "MyClass::MyClass",
      .ParameterList = {"int a", "int   b"},
      .InitList = {"a(a)", "b(b) noexcept"},
      .LineOffset = 0,
      .ColOffset = 11};
  EXPECT_EQ(expected9, extractFuncSpecialMemberInfo(code9));
}

TEST(Extract, FuncOperatorCast) {
  using mergebot::ts::extractFuncOperatorCastInfo;
  using mergebot::ts::FuncOperatorCastInfo;
  // Test Case 1: Template with attributes and initializer list
  std::string code1 = "operator int() { return x; }";
  FuncOperatorCastInfo expected1 = {.OriginalSignature = "operator int() ",
                                    .TemplateParameterList = "",
                                    .Attrs = "",
                                    .BeforeFuncName = "",
                                    .FuncName = "operator int",
                                    .ParameterList = {},
                                    .AfterParameterList = " ",
                                    .LineOffset = 0,
                                    .ColOffset = 0};
  EXPECT_EQ(expected1, extractFuncOperatorCastInfo(code1));

  std::string code2 =
      "template <typename T> [[nodiscard]] [[deprecated]] operator T() { "
      "return x; }";
  FuncOperatorCastInfo expected2 = {
      .OriginalSignature =
          "template <typename T> [[nodiscard]] [[deprecated]] operator T() ",
      .TemplateParameterList = "template <typename T>",
      .Attrs = "[[nodiscard]] [[deprecated]] ",
      .BeforeFuncName = "",
      .FuncName = "operator T",
      .ParameterList = {},
      .AfterParameterList = " ",
      .LineOffset = 0,
      .ColOffset = 51};
  EXPECT_EQ(expected2, extractFuncOperatorCastInfo(code2));

  std::string code3 =
      "[[nodiscard]] operator bool() const noexcept { return x != 0; }";
  FuncOperatorCastInfo expected3 = {
      .OriginalSignature = "[[nodiscard]] operator bool() const noexcept ",
      .TemplateParameterList = "",
      .Attrs = "[[nodiscard]] ",
      .BeforeFuncName = "",
      .FuncName = "operator bool",
      .ParameterList = {},
      .AfterParameterList = " const noexcept ",
      .LineOffset = 0,
      .ColOffset = 14};
  EXPECT_EQ(expected3, extractFuncOperatorCastInfo(code3));

  std::string code7 =
      "[[nodiscard]] constexpr explicit operator int() { return x; }";
  FuncOperatorCastInfo expected7 = {
      .OriginalSignature = "[[nodiscard]] constexpr explicit operator int() ",
      .TemplateParameterList = "",
      .Attrs = "[[nodiscard]] ",
      .BeforeFuncName = "constexpr explicit ",
      .FuncName = "operator int",
      .ParameterList = {},
      .AfterParameterList = " ",
      .LineOffset = 0,
      .ColOffset = 33};
  ASSERT_EQ(expected7, extractFuncOperatorCastInfo(code7));
}

TEST(Extract, FuncDefInfo) {
  using mergebot::ts::extractFuncDefInfo;
  using mergebot::ts::FuncDefInfo;
  // Basic Function
  {
    std::string code = "void foo(int a, float b) {}";
    std::string funcName = "foo";
    FuncDefInfo info = extractFuncDefInfo(code, funcName);

    EXPECT_EQ(info.OriginalSignature, "void foo(int a, float b) ");
    EXPECT_EQ(info.TemplateParameterList, "");
    EXPECT_EQ(info.Attrs, "");
    EXPECT_EQ(info.BeforeFuncName, "void");
    EXPECT_EQ(info.FuncName, "foo");
    EXPECT_EQ(info.ParameterList,
              (std::vector<std::string>{"int a", "float b"}));
    EXPECT_EQ(info.AfterParameterList, " ");
    EXPECT_EQ(info.LineOffset, 0);
    EXPECT_EQ(info.ColOffset, 5);
  }

  // Function with Template
  {
    std::string code = "template <typename T> void foo(T a) {}";
    std::string funcName = "foo";
    FuncDefInfo info = extractFuncDefInfo(code, funcName);

    EXPECT_EQ(info.OriginalSignature, "template <typename T> void foo(T a) ");
    EXPECT_EQ(info.TemplateParameterList, "template <typename T>");
    EXPECT_EQ(info.Attrs, "");
    EXPECT_EQ(info.BeforeFuncName, "void");
    EXPECT_EQ(info.FuncName, "foo");
    EXPECT_EQ(info.ParameterList, std::vector<std::string>{"T a"});
    EXPECT_EQ(info.AfterParameterList, " ");
    EXPECT_EQ(info.LineOffset, 0);
    EXPECT_EQ(info.ColOffset, 27);
  }

  // Function with Attributes
  {
    std::string code = "[[nodiscard]] int foo() noexcept";
    std::string funcName = "foo";
    FuncDefInfo info = extractFuncDefInfo(code, funcName);

    EXPECT_EQ(info.OriginalSignature, "[[nodiscard]] int foo() noexcept");
    EXPECT_EQ(info.TemplateParameterList, "");
    EXPECT_EQ(info.Attrs, "[[nodiscard]] ");
    EXPECT_EQ(info.BeforeFuncName, "int");
    EXPECT_EQ(info.FuncName, "foo");
    EXPECT_TRUE(info.ParameterList.empty());
    EXPECT_EQ(info.AfterParameterList, " noexcept");
    EXPECT_EQ(info.LineOffset, 0);
    EXPECT_EQ(info.ColOffset, 18);
  }

  // Complex Function Name
  {
    std::string code = "int A::B::C::foo() const {}";
    std::string funcName = "A::B::C::foo";
    FuncDefInfo info = extractFuncDefInfo(code, funcName);

    EXPECT_EQ(info.OriginalSignature, "int A::B::C::foo() const ");
    EXPECT_EQ(info.TemplateParameterList, "");
    EXPECT_EQ(info.Attrs, "");
    EXPECT_EQ(info.BeforeFuncName, "int");
    EXPECT_EQ(info.FuncName, "A::B::C::foo");
    EXPECT_TRUE(info.ParameterList.empty());
    EXPECT_EQ(info.AfterParameterList, " const ");
    EXPECT_EQ(info.LineOffset, 0);
    EXPECT_EQ(info.ColOffset, 13);
  }

  // More after Parameter List
  {
    std::string code =
        "virtual int foo(int a, float b) const noexcept override {}";
    std::string funcName = "foo";
    FuncDefInfo info = extractFuncDefInfo(code, funcName);

    EXPECT_EQ(info.OriginalSignature,
              "virtual int foo(int a, float b) const noexcept override ");
    EXPECT_EQ(info.TemplateParameterList, "");
    EXPECT_EQ(info.Attrs, "");
    EXPECT_EQ(info.BeforeFuncName, "virtual int");
    EXPECT_EQ(info.FuncName, "foo");
    EXPECT_EQ(info.ParameterList,
              (std::vector<std::string>{"int a", "float b"}));
    EXPECT_EQ(info.AfterParameterList, " const noexcept override ");
    EXPECT_EQ(info.LineOffset, 0);
    EXPECT_EQ(info.ColOffset, 12);
  }
}

TEST(Extract, GlobalReplace) {
  std::string input_str = "Hello@N@World@S@Example@E@Text";

  // 使用RE2::GlobalReplace进行全局替换
  if (RE2::GlobalReplace(&input_str, "@N@|@S@|@E@", "::")) {
    std::cout << "替换成功: " << input_str << std::endl;
  } else {
    std::cout << "替换失败" << std::endl;
  }
}
