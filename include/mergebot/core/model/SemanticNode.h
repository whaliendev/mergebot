//
// Created by whalien on 08/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_SEMANTICNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_SEMANTICNODE_H

#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/core/model/enum/NodeType.h"
#include "mergebot/core/model/mapping/NodeContext.h"
#include "mergebot/core/sa_utility.h"
#include "mergebot/parser/point.h"
#include "mergebot/parser/range.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>
namespace mergebot {
namespace sa {
class SemanticNode {
public:
  SemanticNode(int NodeId, bool NeedToMerge, NodeType Type,
               const std::string &DisplayName, const std::string &QualifiedName,
               const std::string &OriginalSignature, std::string &&Comment,
               const std::optional<ts::Point> &Point, std::string &&USR);

  friend bool operator==(SemanticNode const &lhs, SemanticNode const &rhs);

  // id in graph
  int ID;
  bool NeedToMerge;

  NodeType Type;
  // identifier extracted
  std::string DisplayName;
  // container qualified name, get from lsp
  std::string QualifiedName;
  /// generalization form of original signature
  std::string OriginalSignature;

  std::string Comment;

  std::optional<ts::Point> StartPoint;

  // the clang-specific "unified symbol resolution" identifier(a clangd
  // extension)
  std::string USR;

  NodeContext Context;
  /// for preserving original format
  int FollowingEOL = 1;

  std::shared_ptr<SemanticNode> Parent;
  std::vector<std::shared_ptr<SemanticNode>> Children;
  // corresponding node comment
};

} // namespace sa
} // namespace mergebot

namespace std {
template <> struct hash<mergebot::sa::SemanticNode> {
  size_t operator()(mergebot::sa::SemanticNode const &Node) const noexcept {
    size_t H = 1;
    mergebot::hash_combine(H, Node.Type);
    mergebot::hash_combine(H, Node.QualifiedName);
    return H;
  }
};
} // namespace std

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_SEMANTICNODE_H
