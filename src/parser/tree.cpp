//
// Created by whalien on 07/05/23.
//

#include "mergebot/parser/tree.h"

#include <tree_sitter/api.h>

#include <vector>

#include "mergebot/parser/tree_cursor.h"

namespace mergebot {
namespace ts {
Tree::Tree(TSTree *tree, const std::string &source, bool keep_text)
    : source_(""),
      // the corresponding parser's lifetime is almost always longer than tree
      tree(tree) {
  if (keep_text) {
    this->source_ = source;
  }
}

Tree::~Tree() { ts_tree_delete(tree); }

Node Tree::rootNode() {
  return Node(ts_tree_root_node(tree), shared_from_this());
}

const std::string &Tree::source() const { return source_; }
TreeCursor Tree::walk() {
  return TreeCursor(Node(ts_tree_root_node(tree), shared_from_this()),
                    shared_from_this());
}

TreeCursor Tree::walk() const { return const_cast<Tree *>(this)->walk(); }

void Tree::edit(TSInputEdit edit) {
  ts_tree_edit(tree, &edit);
  // After editing, the original source has lost its meaning, we here
  // temporarily clear it.
  // TODO(hwa): can we sync source_?
  source_.clear();
}

std::vector<Range> Tree::getChangedRanges(const Tree &new_tree) const {
  uint32_t len = 0;
  TSRange *ranges = ts_tree_get_changed_ranges(tree, new_tree.tree, &len);
  std::vector<Range> ret(ranges, ranges + len);
  free(ranges);
  return ret;
}

void Tree::exportDotGraph(int file_desc) const {
  ts_tree_print_dot_graph(tree, file_desc);
}

}  // namespace ts
}  // namespace mergebot