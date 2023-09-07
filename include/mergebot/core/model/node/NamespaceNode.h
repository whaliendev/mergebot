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
  NamespaceNode(int NodeId, bool NeedToMerge, NodeType Type,
                const std::string &DisplayName,
                const std::string &QualifiedName,
                const std::string &OriginalSignature, std::string &&Comment,
                const std::optional<ts::Point> &Point, std::string &&USR,
                int BeforeFirstChildEOL, std::string &&NSComment)
      : CompositeNode(NodeId, NeedToMerge, Type, DisplayName, QualifiedName,
                      OriginalSignature, std::move(Comment), Point,
                      std::move(USR), BeforeFirstChildEOL),
        NSComment(std::move(NSComment)) {}

  std::string NSComment;
};
} // namespace sa
} // namespace mergebot

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_NAMESPACENODE_H
