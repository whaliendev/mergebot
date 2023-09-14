//
// Created by whalien on 06/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_ENUMNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_ENUMNODE_H

#include "mergebot/core/model/node/CompositeNode.h"

namespace mergebot::sa {
class EnumNode : public CompositeNode {
public:
  std::string EnumKey;
  std::string Attrs;
  std::string EnumBase;

  EnumNode(int NodeId, bool NeedToMerge, NodeKind Kind,
           const std::string &DisplayName, const std::string &QualifiedName,
           const std::string &OriginalSignature, std::string &&Comment,
           const std::optional<ts::Point> &Point, std::string &&USR,
           size_t BeforeFirstChildEOL, const std::string &Key,
           const std::string &Attrs, const std::string &Base,
           bool IsSynthetic = false)
      : CompositeNode(NodeId, NeedToMerge, Kind, DisplayName, QualifiedName,
                      OriginalSignature, std::move(Comment), Point,
                      std::move(USR), BeforeFirstChildEOL, IsSynthetic),
        EnumKey(Key), Attrs(Attrs), EnumBase(Base) {}

  static bool classof(const SemanticNode *N) {
    return N->getKind() == NodeKind::ENUM;
  }
};
} // namespace mergebot::sa

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_ENUMNODE_H
