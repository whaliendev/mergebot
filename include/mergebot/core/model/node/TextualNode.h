//
// Created by whalien on 04/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TEXTUALNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TEXTUALNODE_H

#include "mergebot/core/model/node/TerminalNode.h"
namespace mergebot {
namespace sa {
class TextualNode : public TerminalNode {
public:
  TextualNode(int NodeId, bool NeedToMerge, NodeType Type,
              const std::string &DisplayName, const std::string &QualifiedName,
              const std::string &OriginalSignature, std::string &&Comment,
              const std::optional<ts::Point> &Point, std::string &&USR,
              std::string &&Body, size_t FollowingEOL,
              size_t ParentSignatureHash)
      : TerminalNode(NodeId, NeedToMerge, Type, DisplayName, QualifiedName,
                     OriginalSignature, std::move(Comment), Point,
                     std::move(USR), std::move(Body), FollowingEOL),
        ParentSignatureHash(ParentSignatureHash) {}

  size_t ParentSignatureHash;

  size_t hashSignature() const override {
    size_t H = 1;
    mergebot::hash_combine(H, this->ParentSignatureHash);
    mergebot::hash_combine(H, this->Body);
    return H;
  }
};
} // namespace sa
} // namespace mergebot

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TEXTUALNODE_H
