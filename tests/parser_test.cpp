//
// Created by whalien on 10/05/23.
//

#include "mergebot/parser/parser.h"

#include <gtest/gtest.h>

#include "mergebot/parser/node.h"
#include "mergebot/parser/tree.h"

namespace mergebot {
namespace sa {
TEST(ParserTest, FieldDeclTest) {
  // clang-format off
  std::string source = R"(
class A{
public:
 int a = 3;
 int b = 4;
};
)";
  // clang-format on
  ts::Parser parser(ts::cpp::language());
  std::shared_ptr<ts::Tree> tree = parser.parse(source);
  ts::Node rootNode = tree->rootNode();
  ts::Node ClassNode = rootNode.children[0];
  std::optional<ts::Node> BodyOfClass =
      ClassNode.getChildByFieldName(ts::cpp::fields::field_body.name);
  EXPECT_TRUE(BodyOfClass.has_value());
  std::vector<std::string> childrenSymbolNames;
  for (ts::Node const &Node : BodyOfClass.value().children) {
    childrenSymbolNames.push_back(
        ts::Symbol::nameOfSymbol(ts::cpp::language(), Node.symbol()));
  }
  ASSERT_TRUE(std::any_of(
      childrenSymbolNames.begin(), childrenSymbolNames.end(),
      [](const std::string &SymbolName) {
        return SymbolName == ts::cpp::symbols::sym_field_declaration.name;
      }));
}
}  // namespace sa
}  // namespace mergebot