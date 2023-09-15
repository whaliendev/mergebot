//
// Created by whalien on 08/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TERMINALNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TERMINALNODE_H

#include "mergebot/core/model/SemanticNode.h"
namespace mergebot {
namespace sa {
class TerminalNode : public SemanticNode {
public:
  TerminalNode(int NodeId, bool NeedToMerge, NodeKind Kind,
               const std::string &DisplayName, const std::string &QualifiedName,
               const std::string &OriginalSignature, std::string &&Comment,
               const std::optional<ts::Point> &Point, std::string &&USR,
               std::string &&Body, size_t ParentSignatureHash,
               size_t FollowingEOL, bool IsSynthetic = false)
      : SemanticNode(NodeId, NeedToMerge, Kind, DisplayName, QualifiedName,
                     OriginalSignature, std::move(Comment), Point,
                     std::move(USR), IsSynthetic),
        Body(std::move(Body)), ParentSignatureHash(ParentSignatureHash) {
    this->FollowingEOL = FollowingEOL;
  }

  friend bool operator==(TerminalNode const &lhs, TerminalNode const &rhs) {
    return lhs.Body == rhs.Body;
  }

  size_t hashSignature() const override {
    size_t H = 1;
    mergebot::hash_combine(H, getKind());
    mergebot::hash_combine(H, this->Body);
    return H;
  }

  static bool classof(const SemanticNode *N) {
    return N->getKind() >= NodeKind::TERMINAL_NODE &&
           N->getKind() <= NodeKind::LAST_TERMINAL_NODE;
  }

  std::string Body;
  size_t ParentSignatureHash;
};
} // namespace sa
} // namespace mergebot

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TERMINALNODE_H
