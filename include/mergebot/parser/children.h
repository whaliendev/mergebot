//
// Created by whalien on 07/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_PARSER_CHILDREN_H
#define MB_INCLUDE_MERGEBOT_PARSER_CHILDREN_H

#include <tree_sitter/api.h>

#include <memory>

extern "C" {
namespace mergebot {

namespace ts {

class Node;
class Tree;

class Children {
 public:
  /// note that this method is unsafe, you have to check boundary before
  /// accessing it
  Node operator[](uint32_t) const;

  uint32_t size() const;

  class iterator {
   public:
    //    iterator() = default;

    // Move to next (including current) node with symbol type
    iterator& findSymbol(TSSymbol);

    Node operator*() const;

    /// this implementation is weird
    std::unique_ptr<Node> operator->() const;
    iterator& operator++();

    bool operator==(const iterator& other) const;
    bool operator!=(const iterator& other) const;

    friend class Children;

   private:
    iterator();
    uint32_t index;
    TSNode parent;
    std::shared_ptr<Tree> tree;
    iterator(TSNode, std::shared_ptr<Tree>, uint32_t);
  };

  iterator begin() const;
  iterator end() const;

 private:
  TSNode parent;
  std::shared_ptr<Tree> tree;

  Children(TSNode parent, std::shared_ptr<Tree> tree);

  friend class Node;
};

}  // namespace ts

}  // namespace mergebot
}  // namespace mergebot

#endif  // MB_INCLUDE_MERGEBOT_PARSER_CHILDREN_H
