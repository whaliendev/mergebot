//
// Created by whalien on 07/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_PARSER_NODE_H
#define MB_INCLUDE_MERGEBOT_PARSER_NODE_H

#include <tree_sitter/api.h>

#include <memory>
#include <optional>
#include <vector>

#include "mergebot/parser/children.h"
#include "mergebot/parser/point.h"
#include "mergebot/parser/tree.h"

extern "C" {
namespace mergebot {
namespace ts {

class Tree;
class TreeCursor;
class Children;

class Node {
 public:
  Children children;

  /// we assumed no null TSNode will be used to construct Node,
  /// however, ts_node_child and ts_node_next_sibling will return a null node to
  /// indicate that no such node was found
  Node(TSNode, std::shared_ptr<Tree>);
  ~Node() = default;

  long id() const;
  std::string type() const;
  TSSymbol symbol() const;
  std::string text() const;

  TreeCursor walk() const;

  bool isNamed() const;

  /// check if the node is *extra*. Extra nodes represent things like comments,
  /// which are not required the grammar, but can appear anywhere
  bool isMissing() const;
  /// check if a syntax node has been edited
  bool hasChanges() const;
  /// check if the node is a syntax error or contains any syntax errors
  bool hasError() const;

  /// compare tree, then compare id
  bool operator==(const Node &) const;

  size_t startByte() const;
  size_t endByte() const;

  Point startPoint() const;
  Point endPoint() const;

  size_t childrenCount() const;
  size_t namedChildrenCount() const;

  std::vector<Node> namedChildren();
  std::vector<Node> childrenByFieldID(TSFieldId);
  std::vector<Node> childrenByFieldName(std::string const &name);

  /// get an S-expression representing the node as a string
  std::string sexp() const;

  std::optional<Node> getChildByFieldID(TSFieldId field_id);
  std::optional<Node> getChildByFieldName(std::string const &name);

  std::optional<Node> getChildByFieldID(TSFieldId field_id) const;
  std::optional<Node> getChildByFieldName(std::string const &name) const;

  /// if no field is found, return empty string
  /// note that ts_node_named_child is not implemented, as it's too dangerous,
  /// use namedChildren and index it
  std::optional<std::string> getChildFiledName(size_t index) const;

  std::optional<Node> nextSibling();

  std::optional<Node> nextNamedSibling();
  std::optional<Node> parent();
  std::optional<Node> prevSibling();
  std::optional<Node> prevNamedSibling();

 private:
  TSNode node;
  std::shared_ptr<Tree> tree;

  friend class Children;
  friend class TreeCursor;
};

}  // namespace ts

}  // namespace mergebot
};

extern std::ostream &operator<<(std::ostream &os, const mergebot::ts::Node &);

#endif  // MB_INCLUDE_MERGEBOT_PARSER_NODE_H
