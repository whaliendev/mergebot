//
// Created by whalien on 07/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_PARSER_TREE_CURSOR_H
#define MB_INCLUDE_MERGEBOT_PARSER_TREE_CURSOR_H

#include <tree_sitter/api.h>

#include <memory>
#include <string>

extern "C" {

namespace mergebot {
namespace ts {

class Node;
class Tree;

/// TODO(hwa): refactor this class
class TreeCursor {
 public:
  TreeCursor(Node, std::shared_ptr<Tree>);
  TreeCursor(TreeCursor const&);
  ~TreeCursor();

  Node node();

  /// unsafe, get the field name of the tree cursor's current node
  ///
  /// this returns `NULL` if the current node doesn't have a field. so we return
  //// a const char* instead of std::string
  const char* getCurrentFieldName() const;

  // TODO(hwa): refactor with std::optional
  /// there methods are all unsafe, refactor with optional
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
