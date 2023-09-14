//
// Created by whalien on 06/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_NAMESPACENODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_NAMESPACENODE_H
#include "mergebot/core/model/node/CompositeNode.h"
namespace mergebot {
namespace sa {
class NamespaceNode : public CompositeNode {
public:
  NamespaceNode(int NodeId, bool NeedToMerge, NodeKind Kind,
                const std::string &DisplayName,
                const std::string &QualifiedName,
                const std::string &OriginalSignature, std::string &&Comment,
                const std::optional<ts::Point> &Point, std::string &&USR,
                int BeforeFirstChildEOL, bool IsInline, std::string &&NSComment,
                bool IsSynthetic = false)
      : CompositeNode(NodeId, NeedToMerge, Kind, DisplayName, QualifiedName,
                      OriginalSignature, std::move(Comment), Point,
                      std::move(USR), BeforeFirstChildEOL, IsSynthetic),
        IsInline(IsInline), NSComment(std::move(NSComment)) {}

  static bool classof(const SemanticNode *N) {
    return N->getKind() == NodeKind::NAMESPACE;
  }

  bool IsInline;
  std::string NSComment;
};
} // namespace sa
} // namespace mergebot

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_NAMESPACENODE_H
