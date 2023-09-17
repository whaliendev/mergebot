//
// Created by whalien on 16/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_LINKAGESPECLISTMATCHER_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_LINKAGESPECLISTMATCHER_H
#include "mergebot/core/model/SemanticNode.h"
#include "mergebot/core/model/mapping/TwoWayMatching.h"
#include "mergebot/core/model/node/LinkageSpecNode.h"
#include "mergebot/utils/similarity.h"
#include <memory>
#include <string>

namespace mergebot::sa {
struct LinkageSpecListMatcher {
  void match(TwoWayMatching &Matching,
             std::vector<std::shared_ptr<SemanticNode>> &BaseNodes,
             std::vector<std::shared_ptr<SemanticNode>> &RevisionNodes) {
    for (auto &BaseNode : BaseNodes) {
      for (auto &RevisionNode : RevisionNodes) {
        assert(llvm::isa<LinkageSpecNode>(BaseNode.get()) &&
               llvm::isa<LinkageSpecNode>(RevisionNode.get()));

        auto *Base = llvm::dyn_cast<LinkageSpecNode>(BaseNode.get());
        auto *Revision = llvm::dyn_cast<LinkageSpecNode>(RevisionNode.get());
        if (Base && Revision && isMatched(Base, Revision)) {
          //          spdlog::debug("refactor: {}({}) -> {}({})",
          //          Base->OriginalSignature,
          //                        magic_enum::enum_name(Base->getKind()),
          //                        Revision->OriginalSignature,
          //                        magic_enum::enum_name(Revision->getKind()));
          Matching.OneOneMatching.insert({BaseNode, RevisionNode});

          auto BaseQualifiedName = BaseNode->QualifiedName;
          auto RevisionQualifiedName = RevisionNode->QualifiedName;

          BaseNodes.erase(std::remove_if(BaseNodes.begin(), BaseNodes.end(),
                                         [&](const auto &Node) {
                                           return Node->hashSignature() ==
                                                  BaseNode->hashSignature();
                                         }),
                          BaseNodes.end());

          RevisionNodes.erase(
              std::remove_if(RevisionNodes.begin(), RevisionNodes.end(),
                             [&](const auto &Node) {
                               return Node->hashSignature() ==
                                      RevisionNode->hashSignature();
                             }),
              RevisionNodes.end());
        }
      }
    }
  }

private:
  bool isMatched(const LinkageSpecNode *BaseNode,
                 const LinkageSpecNode *RevisionNode) {
    if (BaseNode->ParentSignatureHash != RevisionNode->ParentSignatureHash) {
      return false;
    }

    // same file, same linkage spec list
    return true;

    //    auto BaseNeighbors = BaseNode->getNearestNeighborNames();
    //    auto RevisionNeighbors = RevisionNode->getNearestNeighborNames();
    //    if (BaseNeighbors.size() >= 2 && RevisionNeighbors.size() >= 2) {
    //      if ((BaseNeighbors[0].empty() && BaseNeighbors[1].empty()) ||
    //          (RevisionNeighbors[0].empty() && RevisionNeighbors[1].empty()))
    //          {
    //        return false;
    //      }
    //      double similarity = util::dice(BaseNeighbors, RevisionNeighbors);
    //      return similarity >= 0.5;
    //    }
    //    return false;
  }
};
} // namespace mergebot::sa
#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_LINKAGESPECLISTMATCHER_H
