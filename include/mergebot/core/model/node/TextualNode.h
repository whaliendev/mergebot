//
// Created by whalien on 04/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TEXTUALNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TEXTUALNODE_H

#include "mergebot/core/model/node/TerminalNode.h"
namespace mergebot {
namespace sa {
// enumerator需要加逗号
class TextualNode : public TerminalNode {
public:
  TextualNode(int NodeId, bool NeedToMerge, NodeKind Kind,
              const std::string &DisplayName, const std::string &QualifiedName,
              const std::string &OriginalSignature, std::string &&Comment,
              const std::optional<ts::Point> &Point, std::string &&USR,
              std::string &&Body, size_t ParentSignatureHash,
              size_t FollowingEOL, const std::string &TUPath, bool IsSynthetic = false)
      : TerminalNode(NodeId, NeedToMerge, Kind, DisplayName, QualifiedName,
                     OriginalSignature, std::move(Comment), Point,
                     std::move(USR), std::move(Body), ParentSignatureHash,
                     FollowingEOL, IsSynthetic), TUPath(TUPath) {}

    size_t hashSignature() const override {
      size_t H = 1;
      //    mergebot::hash_combine(H, this->ParentSignatureHash);
      mergebot::hash_combine(H, this->Body);
      return H;
    }

  static bool classof(const SemanticNode *N) {
    return N->getKind() == NodeKind::TEXTUAL;
  }

  std::string TUPath;
};
} // namespace sa
} // namespace mergebot

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TEXTUALNODE_H
