//
// Created by whalien on 08/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_SEMANTICNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_SEMANTICNODE_H

#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/core/model/enum/AccessSpecifierKind.h"
#include "mergebot/core/model/enum/NodeKind.h"
#include "mergebot/core/model/mapping/NodeContext.h"
#include "mergebot/core/sa_utility.h"
#include "mergebot/parser/point.h"
#include "mergebot/parser/range.h"
#include <llvm/Support/Casting.h> // for LLVM's RTTI template
#include <memory>
#include <optional>
#include <string>
#include <vector>
namespace mergebot {
namespace sa {
class SemanticNode {
public:
  SemanticNode(int NodeId, bool NeedToMerge, NodeKind Kind,
               const std::string &DisplayName, const std::string &QualifiedName,
               const std::string &OriginalSignature, std::string &&Comment,
               const std::optional<ts::Point> &Point, std::string &&USR,
               bool IsSynthetic = false)
      : ID(NodeId), NeedToMerge(NeedToMerge), Kind(Kind),
        DisplayName(DisplayName),
        QualifiedName(getFullQualifiedName(QualifiedName, DisplayName, Kind)),
        OriginalSignature(OriginalSignature), Comment(Comment),
        StartPoint(Point), USR(USR), AccessSpecifier(AccessSpecifierKind::None),
        IsSynthetic(IsSynthetic) {}

  friend bool operator==(SemanticNode const &lhs, SemanticNode const &rhs) {
    return lhs.hashSignature() == rhs.hashSignature();
  }

  virtual size_t hashSignature() const = 0;

  virtual ~SemanticNode() = default;

  std::vector<std::string> getNearestNeighborNames() const {
    if (auto ParentPtr = Parent.lock()) {
      std::string PrevNeighbor;
      std::string NextNeighbor;
      SemanticNode *prevSibling = nullptr;
      bool Found = false;
      for (auto &Child : ParentPtr->Children) {
        if (Found) {
          NextNeighbor = Child->QualifiedName;
          break;
        }
        if (Child->hashSignature() == this->hashSignature()) {
          Found = true;
          if (prevSibling) {
            PrevNeighbor = prevSibling->QualifiedName;
          }
        }
        prevSibling = Child.get();
      }
      return {PrevNeighbor, NextNeighbor};
    }
    return {"", ""};
  }

public:
  // id in graph
  int ID;
  bool NeedToMerge;

protected:
  const NodeKind Kind;

public:
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
  int FollowingEOL = 0;

  AccessSpecifierKind AccessSpecifier = AccessSpecifierKind::None;

  bool IsSynthetic;

  std::weak_ptr<SemanticNode> Parent;
  std::vector<std::shared_ptr<SemanticNode>> Children;

  NodeKind getKind() const { return Kind; }

  static bool classof(const SemanticNode *N) {
    return N->getKind() >= NodeKind::NODE && N->getKind() <= NodeKind::COUNT;
  }
};

struct SemanticNodeHasher {
  size_t
  operator()(const std::shared_ptr<mergebot::sa::SemanticNode> &node) const {
    return node->hashSignature();
  }
};

struct SemanticNodeEqual {
  bool
  operator()(const std::shared_ptr<mergebot::sa::SemanticNode> &lhs,
             const std::shared_ptr<mergebot::sa::SemanticNode> &rhs) const {
    return *lhs == *rhs;
  }
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
