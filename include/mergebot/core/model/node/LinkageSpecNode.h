//
// Created by whalien on 08/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_LINKAGESPECNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_LINKAGESPECNODE_H

#include "mergebot/core/model/node/CompositeNode.h"

namespace mergebot::sa {
class LinkageSpecNode : public CompositeNode {
public:
  LinkageSpecNode(int NodeId, bool NeedToMerge, NodeKind Kind,
                  const std::string &DisplayName,
                  const std::string &QualifiedName,
                  const std::string &OriginalSignature, std::string &&Comment,
                  const std::optional<ts::Point> &Point, std::string &&USR,
                  size_t BeforeFirstChildEOLs, size_t ParentSignatureHash,
                  bool IsSynthetic = false)
      : CompositeNode(NodeId, NeedToMerge, Kind, DisplayName, QualifiedName,
                      OriginalSignature, std::move(Comment), Point,
                      std::move(USR), BeforeFirstChildEOLs, IsSynthetic),
        ParentSignatureHash(ParentSignatureHash) {}

  size_t hashSignature() const override {
    size_t H = 1;
    mergebot::hash_combine(H, this->ParentSignatureHash);
    mergebot::hash_combine(H, this->DisplayName);
    return H;
  }

  static bool classof(const SemanticNode *N) {
    return N->getKind() == NodeKind::LINKAGE_SPEC_LIST;
  }

  size_t ParentSignatureHash;
};
} // namespace mergebot::sa

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_LINKAGESPECNODE_H
