//
// Created by whalien on 16/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_TRANSLATIONUNITMATCHER_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_TRANSLATIONUNITMATCHER_H

#include "mergebot/core/model/SemanticNode.h"
#include "mergebot/core/model/mapping/TwoWayMatching.h"
#include "mergebot/core/model/node/TranslationUnitNode.h"
#include <vector>
namespace mergebot {
namespace sa {
struct TranslationUnitMatcher {
  void match(TwoWayMatching &Matching,
             std::vector<std::shared_ptr<SemanticNode>> &BaseNodes,
             std::vector<std::shared_ptr<SemanticNode>> &RevisionNodes) {
    for (auto &BaseNode : BaseNodes) {
      for (auto &RevisionNode : RevisionNodes) {
        assert(llvm::isa<TranslationUnitNode>(BaseNode.get()) &&
               llvm::isa<TranslationUnitNode>(RevisionNode.get()));
        if (BaseNode->QualifiedName == RevisionNode->QualifiedName) {
          //          spdlog::debug("refactor: {}({}) -> {}({})",
          //                        BaseNode->OriginalSignature,
          //                        magic_enum::enum_name(BaseNode->getKind()),
          //                        RevisionNode->OriginalSignature,
          //                        magic_enum::enum_name(RevisionNode->getKind()));
          Matching.OneOneMatching.insert({BaseNode, RevisionNode});
          // take care of equality match of std::shared_ptr<SemanticNode>
          BaseNodes.erase(std::remove_if(BaseNodes.begin(), BaseNodes.end(),
                                         [&](auto &Node) {
                                           return Node->QualifiedName ==
                                                  BaseNode->QualifiedName;
                                         }),
                          BaseNodes.end());
          RevisionNodes.erase(
              std::remove_if(RevisionNodes.begin(), RevisionNodes.end(),
                             [&](auto &Node) {
                               return Node->QualifiedName ==
                                      RevisionNode->QualifiedName;
                             }),
              RevisionNodes.end());
        }
      }
    }
  }
};
} // namespace sa
} // namespace mergebot

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_TRANSLATIONUNITMATCHER_H
