//
// Created by whalien on 17/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_ENUMMATCHER_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_ENUMMATCHER_H

#include "mergebot/core/model/SemanticNode.h"
#include "mergebot/core/model/mapping/TwoWayMatching.h"
#include "mergebot/core/model/node/EnumNode.h"
#include "mergebot/globals.h"
#include "mergebot/utils/similarity.h"
#include <memory>
#include <vector>
namespace mergebot::sa {
struct EnumMatcher {
  void match(TwoWayMatching &Matching,
             std::vector<std::shared_ptr<SemanticNode>> &BaseNodes,
             std::vector<std::shared_ptr<SemanticNode>> &RevisionNodes) {
    for (auto &BaseNode : BaseNodes) {
      for (auto &RevisionNode : RevisionNodes) {
        assert(llvm::isa<EnumNode>(BaseNode.get()) &&
               llvm::isa<EnumNode>(RevisionNode.get()));

        auto *Base = llvm::dyn_cast<EnumNode>(BaseNode.get());
        auto *Revision = llvm::dyn_cast<EnumNode>(RevisionNode.get());
        if (Base && Revision && isMatched(Base, Revision)) {
          spdlog::debug("refactor: {}({}) -> {}({})", Base->QualifiedName,
                        magic_enum::enum_name(Base->getKind()),
                        Revision->QualifiedName,
                        magic_enum::enum_name(Revision->getKind()));
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
  bool isMatched(const EnumNode *BaseNode, const EnumNode *RevisionNode) {
    double NameSim = 0, BaseSim = 0, BodySim = 0;
    NameSim = util::string_cosine(BaseNode->QualifiedName,
                                  RevisionNode->QualifiedName);
    if (NameSim < 0) {
      NameSim = 0;
    }

    BaseSim = util::string_cosine(BaseNode->EnumBase, RevisionNode->EnumBase);
    if (BaseSim < 0) {
      BaseSim = 0;
    }

    BodySim = util::string_cosine(BaseNode->Body, RevisionNode->Body);
    if (BodySim < 0) {
      BodySim = 0;
    }

    return NameSim * 0.2 + BaseSim * 0.2 + BodySim * 0.6 >= MIN_SIMI;
  }
};
} // namespace mergebot::sa

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_ENUMMATCHER_H
