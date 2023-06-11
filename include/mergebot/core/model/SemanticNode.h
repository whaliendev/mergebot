//
// Created by whalien on 08/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_SEMANTICNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_SEMANTICNODE_H

#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/core/model/enum/NodeType.h"
#include "mergebot/core/model/mapping/NodeContext.h"
#include "mergebot/core/sa_utility.h"
#include "mergebot/parser/range.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>
namespace mergebot {
namespace sa {
class NodeContext;

class SemanticNode {
public:
  int ID;
  NodeContext Context;
  /// for preserving original format
  int FollowBlankLines = 1;

  friend bool operator==(SemanticNode const &lhs, SemanticNode const &rhs);

  NodeType nodeType() const { return Type; }

  std::string qualifiedName() const { return QualifiedName; }

private:
  bool NeedMerge;
  std::shared_ptr<SemanticNode> Parent;
  std::vector<std::shared_ptr<SemanticNode>> Children;
  NodeType Type;
  std::string DisplayName;
  std::string QualifiedName;
  /// generalization form of original signature
  std::string OriginalSignature;
  /// whether the node is defined inside the graph or not
  bool IsInternal;
  std::optional<ts::Range> range;
};

} // namespace sa
} // namespace mergebot

namespace std {
template <> struct hash<mergebot::sa::SemanticNode> {
  size_t operator()(mergebot::sa::SemanticNode const &Node) const noexcept {
    size_t H = 1;
    mergebot::hash_combine(H, Node.nodeType());
    mergebot::hash_combine(H, Node.qualifiedName());
    return H;
  }
};
} // namespace std

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_SEMANTICNODE_H
