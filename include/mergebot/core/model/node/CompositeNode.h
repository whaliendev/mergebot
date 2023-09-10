//
// Created by whalien on 08/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_COMPOSITENODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_COMPOSITENODE_H

#include "mergebot/core/model/SemanticNode.h"
namespace mergebot {
namespace sa {
class CompositeNode : public SemanticNode {
public:
  size_t BeforeFirstChildEOL = 0;

  CompositeNode(int NodeId, bool NeedToMerge, NodeType Type,
                const std::string &DisplayName,
                const std::string &QualifiedName,
                const std::string &OriginalSignature, std::string &&Comment,
                const std::optional<ts::Point> &Point, std::string &&USR,
                size_t BeforeFirstChildEOL)
      : SemanticNode(NodeId, NeedToMerge, Type, DisplayName, QualifiedName,
                     OriginalSignature, std::move(Comment), Point,
                     std::move(USR)),
        BeforeFirstChildEOL(BeforeFirstChildEOL) {}

  size_t hashSignature() const override {
    size_t H = 1;
    if (!this->USR.empty()) { // USR is the most important
      mergebot::hash_combine(H, this->USR);
      return H;
    }
    // visibility change is a refactoring, we intentionally don't count it
    //    if (AccessSpecifier != AccessSpecifierKind::None) {
    //
    //    }
    mergebot::hash_combine(H, this->Type);
    mergebot::hash_combine(H, this->QualifiedName + this->DisplayName);
    return H;
  }
};
} // namespace sa
} // namespace mergebot

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_COMPOSITENODE_H
