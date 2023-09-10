//
// Created by whalien on 08/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_FIELDDECLARATIONNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_FIELDDECLARATIONNODE_H
#include "mergebot/core/model/node/TerminalNode.h"
namespace mergebot::sa {
class FieldDeclarationNode : public TerminalNode {
public:
  FieldDeclarationNode(int NodeId, bool NeedToMerge, NodeKind Kind,
                       const std::string &DisplayName,
                       const std::string &QualifiedName,
                       const std::string &OriginalSignature,
                       std::string &&Comment,
                       const std::optional<ts::Point> &Point, std::string &&USR,
                       std::string &&Body, size_t FollowingEOL,
                       std::string &&Declarator, size_t ParentSignatureHash)
      : TerminalNode(NodeId, NeedToMerge, Kind, DisplayName, QualifiedName,
                     OriginalSignature, std::move(Comment), Point,
                     std::move(USR), std::move(Body), FollowingEOL),
        Declarator(std::move(Declarator)),
        ParentSignatureHash(ParentSignatureHash) {}

  size_t hashSignature() const override {
    size_t H = 1;
    mergebot::hash_combine(H, this->ParentSignatureHash);
    //    mergebot::hash_combine(H, this->Body);
    mergebot::hash_combine(H, this->Declarator);
    return H;
  }

  static bool classof(const SemanticNode *N) {
    return N->getKind() == NodeKind::FIELD_DECLARATION;
  }

  std::string Declarator;
  std::size_t ParentSignatureHash;
};
} // namespace mergebot::sa
#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_FIELDDECLARATIONNODE_H