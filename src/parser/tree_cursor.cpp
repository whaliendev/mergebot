//
// Created by whalien on 07/05/23.
//

#include "mergebot/parser/tree_cursor.h"

#include <tree_sitter/api.h>

#include "mergebot/parser/node.h"

namespace mergebot {
namespace ts {
TreeCursor::TreeCursor(Node node, std::shared_ptr<Tree> tree)
    // call copy ctor of Node
    : cursor(ts_tree_cursor_new(node.node)),
      node_(new Node(node)),
      tree(tree) {}

TreeCursor::TreeCursor(TreeCursor const& other)
    : cursor(ts_tree_cursor_copy(&other.cursor)),
      node_(nullptr),
      tree(other.tree) {}

TreeCursor::~TreeCursor() {
  delete node_;
  ts_tree_cursor_delete(&cursor);
}
Node TreeCursor::node() {
  if (!node_) {
    node_ = new Node(ts_tree_cursor_current_node(&cursor), tree);
  }
  return *node_;
}

const char* TreeCursor::getCurrentFieldName() const {
  return ts_tree_cursor_current_field_name(&cursor);
}

bool TreeCursor::gotoParent() {
  bool result = ts_tree_cursor_goto_parent(&cursor);
  if (result) {
    node_ = nullptr;
  }
  return result;
}
bool TreeCursor::gotoFirstChild() {
  bool result = ts_tree_cursor_goto_first_child(&cursor);
  if (result) {
    node_ = nullptr;
  }
  return result;
}

bool TreeCursor::gotoNextSibling() {
  bool result = ts_tree_cursor_goto_next_sibling(&cursor);
  if (result) {
    node_ = nullptr;
  }
  return result;
}
}  // namespace ts
}  // namespace mergebot