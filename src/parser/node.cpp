//
// Created by whalien on 07/05/23.
//

#include "mergebot/parser/node.h"

#include <tree_sitter/api.h>

#include <cassert>
#include <cstddef>
#include <optional>

#include "mergebot/parser/tree_cursor.h"

namespace mergebot {
namespace ts {
static TSTreeCursor cursor;
Node::Node(TSNode node, std::shared_ptr<Tree> tree)
    : children(node, tree), node(node), tree(tree) {
  assert(!ts_node_is_null(node) && "node should not be null");
}

std::string Node::type() const { return ts_node_type(node); }
TSSymbol Node::symbol() const { return ts_node_symbol(node); }
size_t Node::startByte() const { return ts_node_start_byte(node); }
size_t Node::endByte() const { return ts_node_end_byte(node); }
ts::Point Node::startPoint() const { return ts_node_start_point(node); }
ts::Point Node::endPoint() const { return ts_node_end_point(node); }
std::string Node::sexp() const {
  char *sexp = ts_node_string(node);
  std::string ret(sexp);
  free(sexp);
  return ret;
}
bool Node::isNamed() const { return ts_node_is_named(node); }
bool Node::isMissing() const { return ts_node_is_missing(node); }
bool Node::hasChanges() const { return ts_node_has_changes(node); }
bool Node::hasError() const { return ts_node_has_error(node); }
std::optional<Node> Node::parent() {
  TSNode nodeOrNull = ts_node_parent(node);
  if (ts_node_is_null(nodeOrNull)) {
    return std::nullopt;
  }
  return Node(nodeOrNull, tree);
}
std::optional<std::string> Node::getChildFiledName(size_t index) const {
  const char *name = ts_node_field_name_for_child(node, index);
  if (!name) {
    return std::nullopt;
  }
  return name;
}
size_t Node::childrenCount() const { return ts_node_child_count(node); }
size_t Node::namedChildrenCount() const {
  return ts_node_named_child_count(node);
}

std::vector<Node> Node::namedChildren() {
  size_t named_cnt = ts_node_named_child_count(node);
  std::vector<Node> ret;
  ret.reserve(named_cnt);
  for (const auto &child : children) {
    if (child.isNamed()) {
      // copy ctor called here
      ret.emplace_back(child);
    }
  }
  return ret;
}

std::vector<Node> Node::childrenByFieldID(TSFieldId id) {
  std::vector<Node> result;
  ts_tree_cursor_reset(&cursor, node);
  int ok = ts_tree_cursor_goto_first_child(&cursor);
  while (ok) {
    if (ts_tree_cursor_current_field_id(&cursor) == id) {
      result.emplace_back(Node(ts_tree_cursor_current_node(&cursor), tree));
    }
    ok = ts_tree_cursor_goto_next_sibling(&cursor);
  }

  return result;
}

std::vector<Node> Node::childrenByFieldName(const std::string &name) {
  const TSLanguage *lang = ts_tree_language(tree->tree);
  TSFieldId field_id =
      ts_language_field_id_for_name(lang, name.data(), name.length());
  return childrenByFieldID(field_id);
}

std::optional<Node> Node::getChildByFieldID(TSFieldId field_id) {
  TSNode nodeOrNull = ts_node_child_by_field_id(node, field_id);
  if (ts_node_is_null(nodeOrNull)) {
    return std::nullopt;
  }
  return Node(nodeOrNull, tree);
}

std::optional<Node> Node::getChildByFieldName(const std::string &name) {
  TSNode nodeOrNull =
      ts_node_child_by_field_name(node, name.c_str(), name.length());
  if (ts_node_is_null(nodeOrNull)) {
    return std::nullopt;
  }
  return Node(nodeOrNull, tree);
}

std::optional<Node> Node::getChildByFieldID(TSFieldId field_id) const {
  TSNode nodeOrNull = ts_node_child_by_field_id(node, field_id);
  if (ts_node_is_null(nodeOrNull)) {
    return std::nullopt;
  }
  return Node(ts_node_child_by_field_id(node, field_id), tree);
}

std::optional<Node> Node::getChildByFieldName(const std::string &name) const {
  TSNode nodeOrNull =
      ts_node_child_by_field_name(node, name.c_str(), name.length());
  if (ts_node_is_null(nodeOrNull)) {
    return std::nullopt;
  }
  return Node(nodeOrNull, tree);
}

std::optional<Node> Node::nextSibling() {
  TSNode nodeOrNull = ts_node_next_sibling(node);
  if (ts_node_is_null(nodeOrNull)) {
    return std::nullopt;
  }
  return Node(nodeOrNull, tree);
}

std::optional<Node> Node::nextNamedSibling() {
  TSNode nodeOrNull = ts_node_next_named_sibling(node);
  if (ts_node_is_null(nodeOrNull)) {
    return std::nullopt;
  }
  return Node(nodeOrNull, tree);
}

std::optional<Node> Node::prevSibling() {
  TSNode nodeOrNull = ts_node_prev_sibling(node);
  if (ts_node_is_null(nodeOrNull)) {
    return std::nullopt;
  }
  return Node(nodeOrNull, tree);
}

std::optional<Node> Node::prevNamedSibling() {
  TSNode nodeOrNull = ts_node_prev_named_sibling(node);
  if (ts_node_is_null(nodeOrNull)) {
    return std::nullopt;
  }
  return Node(nodeOrNull, tree);
}

bool Node::operator==(const Node &rhs) const {
  // id is unique in unique tree
  return this->id() == rhs.id() && this->tree == rhs.tree;
}

std::string Node::text() const {
  return tree->source().substr(startByte(), endByte() - startByte());
}

long Node::id() const { return *static_cast<const long *>(node.id); }

TreeCursor Node::walk() const { return TreeCursor(*this, tree); }
}  // namespace ts
}  // namespace mergebot

std::ostream &operator<<(std::ostream &os, const mergebot::ts::Node &node) {
  if (node.isNamed()) {
    return os << "Node(type=" << node.type()
              << ", start_point=" << node.startPoint() << ", "
              << node.endPoint() << ')';
  } else {
    // terminal node
    return os << "Node(type=\"" << node.type()
              << "\", start_point=" << node.startPoint() << ", "
              << node.endPoint() << ')';
  }
}