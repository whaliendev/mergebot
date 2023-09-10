//
// Created by whalien on 03/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_ORPHANCOMMENTNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_ORPHANCOMMENTNODE_H

#include "mergebot/core/model/node/TerminalNode.h"
namespace mergebot::sa {
class OrphanCommentNode : public TerminalNode {
public:
  OrphanCommentNode(int NodeId, bool NeedToMerge, NodeType Type,
                    const std::string &DisplayName,
                    const std::string &QualifiedName,
                    const std::string &OriginalSignature, std::string &&Comment,
                    const std::optional<ts::Point> &Point, std::string &&USR,
                    std::string &&Body, size_t FollowingEOL)
      : TerminalNode(NodeId, NeedToMerge, Type, DisplayName, QualifiedName,
                     OriginalSignature, std::move(Comment), Point,
                     std::move(USR), std::move(Body), FollowingEOL) {}
};
} // namespace mergebot::sa

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_ORPHANCOMMENTNODE_H
