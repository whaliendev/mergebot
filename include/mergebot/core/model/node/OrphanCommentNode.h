//
// Created by whalien on 03/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_ORPHANCOMMENTNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_ORPHANCOMMENTNODE_H

#include "mergebot/core/model/node/TerminalNode.h"
namespace mergebot::sa {
class OrphanCommentNode : public TerminalNode {
public:
  OrphanCommentNode(int NodeId, bool NeedToMerge, NodeKind Kind,
                    const std::string &DisplayName,
                    const std::string &QualifiedName,
                    const std::string &OriginalSignature, std::string &&Comment,
                    const std::optional<ts::Point> &Point, std::string &&USR,
                    std::string &&Body, size_t ParentSignatureHash,
                    size_t FollowingEOL, bool IsSynthetic = false)
      : TerminalNode(NodeId, NeedToMerge, Kind, DisplayName, QualifiedName,
                     OriginalSignature, std::move(Comment), Point,
                     std::move(USR), std::move(Body), ParentSignatureHash,
                     FollowingEOL, IsSynthetic) {}

  static bool classof(const SemanticNode *N) {
    return N->getKind() == NodeKind::ORPHAN_COMMENT;
  }
};
} // namespace mergebot::sa

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_ORPHANCOMMENTNODE_H
