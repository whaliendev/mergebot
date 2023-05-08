//
// Created by whalien on 07/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_PARSER_TREE_CURSOR_H
#define MB_INCLUDE_MERGEBOT_PARSER_TREE_CURSOR_H

#include <tree_sitter/api.h>

#include <memory>
#include <optional>
#include <string>

extern "C" {

namespace mergebot {
namespace ts {

class Node;
class Tree;

class TreeCursor {
 public:
  TreeCursor(Node, std::shared_ptr<Tree>);
  TreeCursor(TreeCursor const&);
  ~TreeCursor();

  Node node();

  std::optional<std::string> getCurrentFieldName() const;

  // TODO(hwa): is it unsafe? do we need to refactor to std::optional<Node>?
  bool gotoParent();
  bool gotoFirstChild();
  bool gotoNextSibling();

 private:
  TSTreeCursor cursor;
  Node* node_;
  std::shared_ptr<Tree> tree;
};

}  // namespace ts
}  // namespace mergebot
}

#endif  // MB_INCLUDE_MERGEBOT_PARSER_TREE_CURSOR_H
