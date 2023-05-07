//
// Created by whalien on 07/05/23.
//

#ifndef MB_PARSER_TREE_H
#define MB_PARSER_TREE_H

#include <tree_sitter/api.h>

#include <memory>
#include <string>
#include <vector>

#include "mergebot/parser/children.h"
#include "mergebot/parser/languages/cpp.h"
#include "mergebot/parser/node.h"
#include "mergebot/parser/parser.h"
#include "mergebot/parser/range.h"
#include "mergebot/parser/tree_cursor.h"
namespace mergebot {
namespace ts {

class Tree : public std::enable_shared_from_this<Tree> {
 public:
  Tree(TSTree* tree, std::string const& source, bool keep_text);
  ~Tree();

  static std::shared_ptr<Tree> create(TSTree* tree, std::string const& source,
                                      bool keep_text) {
    return std::make_shared<Tree>(tree, source, keep_text);
  }

  Node rootNode();

  /// not implemented, as its ambiguous
  //  Node rootNodeWithOffset(uint32_t offset_bytes, Point offset_point) const;

  /// not implemented, no requirement
  //  const TSLanguage* treeLanguage(const Tree*) const;

  /// not implemented, no need
  // the returned pointer must be freed by the caller
  //  std::vector<Range> getIncludedRanges() const;

  const std::string& source() const;

  TreeCursor walk();
  TreeCursor walk() const;

  void edit(TSInputEdit);

  std::vector<Range> getChangedRanges(const Tree& new_tree) const;

  void exportDotGraph(int file_desc) const;

 private:
  std::string source_;
  TSTree* tree;

  friend class Parser;
  friend class Node;
};

}  // namespace ts
}  // namespace mergebot

#endif  // MB_PARSER_TREE_H
