//
// Created by whalien on 08/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_SEMANTICNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_SEMANTICNODE_H

#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/core/model/enum/AccessSpecifierKind.h"
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
               const std::optional<ts::Point> &Point, std::string &&USR)
      : ID(NodeId), NeedToMerge(NeedToMerge), Type(Type),
        DisplayName(DisplayName), QualifiedName(QualifiedName),
        OriginalSignature(OriginalSignature), Comment(Comment),
        StartPoint(Point), USR(USR),
        AccessSpecifier(AccessSpecifierKind::None) {}

  friend bool operator==(SemanticNode const &lhs, SemanticNode const &rhs) {
    return lhs.hashSignature() == rhs.hashSignature();
  }

  virtual size_t hashSignature() const = 0;

  virtual ~SemanticNode() = default;

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

  AccessSpecifierKind AccessSpecifier = AccessSpecifierKind::None;

  std::vector<std::shared_ptr<SemanticNode>> Children;
};

} // namespace sa
} // namespace mergebot

namespace std {
template <> struct hash<mergebot::sa::SemanticNode> {
  size_t operator()(mergebot::sa::SemanticNode const &Node) const noexcept {
    return Node.hashSignature();
  }
};
} // namespace std

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_SEMANTICNODE_H
