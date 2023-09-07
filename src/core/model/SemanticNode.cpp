//
// Created by whalien on 08/05/23.
//

#include "mergebot/core/model/SemanticNode.h"

namespace mergebot {
namespace sa {
SemanticNode::SemanticNode(int NodeId, bool NeedToMerge, NodeType Type,
                           const std::string &DisplayName,
                           const std::string &QualifiedName,
                           const std::string &OriginalSignature,
                           std::string &&Comment,
                           const std::optional<ts::Point> &Point,
                           std::string &&USR)
    : ID(NodeId), NeedToMerge(NeedToMerge), Type(Type),
      DisplayName(DisplayName), QualifiedName(QualifiedName),
      OriginalSignature(OriginalSignature), Comment(Comment), StartPoint(Point),
      USR(USR) {}

bool operator==(const SemanticNode &lhs, const SemanticNode &rhs) {
  // TODO(hwa): to be implemented
  return false;
}
} // namespace sa
} // namespace mergebot