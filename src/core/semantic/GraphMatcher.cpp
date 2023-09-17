//
// Created by whalien on 30/04/23.
//

#include "mergebot/core/semantic/GraphMatcher.h"
#include "mergebot/core/model/matcher/EnumMatcher.h"
#include "mergebot/core/model/matcher/FieldDeclMatcher.h"
#include "mergebot/core/model/matcher/LinkageSpecListMatcher.h"
#include "mergebot/core/model/matcher/TranslationUnitMatcher.h"
#include "mergebot/core/model/matcher/TypeSpecifierMatcher.h"

// #define MB_DEBUG

namespace mergebot::sa {
void GraphMatcher::topDownMatch() {
  std::unordered_map<size_t, std::shared_ptr<SemanticNode>> BaseNodes;
  std::unordered_map<size_t, std::shared_ptr<SemanticNode>> RevisionNodes;
  for (auto VDesc : boost::make_iterator_range(boost::vertices(BaseGraph))) {
    auto Node = BaseGraph[VDesc];
    // we only care about nodes that need to be merged,
    // as other nodes are context information
    if (Node->NeedToMerge && !Node->IsSynthetic) {
      BaseNodes[Node->hashSignature()] = Node;
    }
  }
  for (auto VDesc : boost::make_iterator_range(vertices(RevisionGraph))) {
    auto Node = RevisionGraph[VDesc];
    if (Node->NeedToMerge && !Node->IsSynthetic) {
      RevisionNodes[Node->hashSignature()] = Node;
    }
  }
  for (auto &[Hash, Node] : BaseNodes) {
    if (RevisionNodes.find(Hash) != RevisionNodes.end()) {
      Matching.OneOneMatching.insert({Node, RevisionNodes[Hash]});
      RevisionNodes.erase(Hash);
    } else {
      // possibly deleted
      Matching.addUnmatchedNode(Node, true);
    }
  }
  // possibly added
  std::for_each(RevisionNodes.begin(), RevisionNodes.end(), [&](auto &Pair) {
    Matching.addUnmatchedNode(Pair.second, false);
  });
}

void GraphMatcher::bottomUpMatch() {
  /// Assume that only nodes of the same type are allowed to match.
  // translation unit
  std::vector<std::shared_ptr<SemanticNode>> &BaseUnmatchedTUs =
      Matching.PossiblyDeleted[NodeKind::TRANSLATION_UNIT];
  std::vector<std::shared_ptr<SemanticNode>> &RevisionUnmatchedTUs =
      Matching.PossiblyAdded[NodeKind::TRANSLATION_UNIT];
  if (BaseUnmatchedTUs.size() && RevisionUnmatchedTUs.size()) {
    TranslationUnitMatcher TUMatcher;
    TUMatcher.match(Matching, BaseUnmatchedTUs, RevisionUnmatchedTUs);
  }

  /// linkage spec list
  std::vector<std::shared_ptr<SemanticNode>> &BaseUnmatchedLinkageSpecs =
      Matching.PossiblyDeleted[NodeKind::LINKAGE_SPEC_LIST];
  std::vector<std::shared_ptr<SemanticNode>> &RevisionUnmatchedLinkageSpecs =
      Matching.PossiblyAdded[NodeKind::LINKAGE_SPEC_LIST];
  if (BaseUnmatchedLinkageSpecs.size() &&
      RevisionUnmatchedLinkageSpecs.size()) {
    LinkageSpecListMatcher LSLMatcher;
    LSLMatcher.match(Matching, BaseUnmatchedLinkageSpecs,
                     RevisionUnmatchedLinkageSpecs);
  }

  /// namespace doesn't need to do similarity match

  /// type class, struct, union
  std::vector<std::shared_ptr<SemanticNode>> &BaseUnmatchedTypes =
      Matching.PossiblyDeleted[NodeKind::TYPE];
  std::vector<std::shared_ptr<SemanticNode>> &RevisionUnmatchedTypes =
      Matching.PossiblyAdded[NodeKind::TYPE];
  if (BaseUnmatchedTypes.size() && RevisionUnmatchedTypes.size()) {
    TypeSpecifierMatcher TypeMatcher;
    TypeMatcher.match(Matching, BaseUnmatchedTypes, RevisionUnmatchedTypes);
  }

  /// enum
  std::vector<std::shared_ptr<SemanticNode>> &BaseUnmatchedEnums =
      Matching.PossiblyDeleted[NodeKind::Enum];
  std::vector<std::shared_ptr<SemanticNode>> &RevisionUnmatchedEnums =
      Matching.PossiblyAdded[NodeKind::Enum];
  if (BaseUnmatchedEnums.size() && RevisionUnmatchedEnums.size()) {
    EnumMatcher EMatcher;
    EMatcher.match(Matching, BaseUnmatchedEnums, RevisionUnmatchedEnums);
  }

  //
  std::vector<std::shared_ptr<SemanticNode>> &BaseUnmatchedFields =
      Matching.PossiblyDeleted[NodeKind::FIELD_DECLARATION];
  std::vector<std::shared_ptr<SemanticNode>> &RevisionUnmatchedFields =
      Matching.PossiblyAdded[NodeKind::FIELD_DECLARATION];
  if (BaseUnmatchedFields.size() && RevisionUnmatchedFields.size()) {
    FieldDeclMatcher FDMatcher;
    FDMatcher.match(Matching, BaseUnmatchedFields, RevisionUnmatchedFields);
  }
}

TwoWayMatching GraphMatcher::match() {
  topDownMatch();
#ifdef MB_DEBUG
  spdlog::debug("one one matching size: {}", Matching.OneOneMatching.size());
  for (auto &[BaseNode, RevisionNode] : Matching.OneOneMatching) {
    spdlog::debug("kind: {}, matching: {} -> {}",
                  magic_enum::enum_name(BaseNode->getKind()),
                  BaseNode->QualfiedName, RevisionNode->QualifiedName);
  }
#endif
  bottomUpMatch();
  return Matching;
}
} // namespace mergebot::sa
