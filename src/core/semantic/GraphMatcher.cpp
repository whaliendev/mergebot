//
// Created by whalien on 30/04/23.
//

#include "mergebot/core/semantic/GraphMatcher.h"
#include "mergebot/core/model/matcher/EnumMatcher.h"
#include "mergebot/core/model/matcher/FieldDeclMatcher.h"
#include "mergebot/core/model/matcher/FuncDefMathcer.h"
#include "mergebot/core/model/matcher/FuncSpecialMemberMatcher.h"
#include "mergebot/core/model/matcher/LinkageSpecListMatcher.h"
#include "mergebot/core/model/matcher/TranslationUnitMatcher.h"
#include "mergebot/core/model/matcher/TypeSpecifierMatcher.h"
#include "mergebot/core/sa_utility.h"

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

  /// TODO(hwa): add body similarity calc, linkage spec list
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
  std::unordered_map<size_t, size_t> RefactoredTypes;
  if (BaseUnmatchedTypes.size() && RevisionUnmatchedTypes.size()) {
    TypeSpecifierMatcher TypeMatcher;
    TypeMatcher.match(Matching, BaseUnmatchedTypes, RevisionUnmatchedTypes,
                      RefactoredTypes);
  }

  /// enum
  std::vector<std::shared_ptr<SemanticNode>> &BaseUnmatchedEnums =
      Matching.PossiblyDeleted[NodeKind::ENUM];
  std::vector<std::shared_ptr<SemanticNode>> &RevisionUnmatchedEnums =
      Matching.PossiblyAdded[NodeKind::ENUM];
  if (BaseUnmatchedEnums.size() && RevisionUnmatchedEnums.size()) {
    EnumMatcher EMatcher;
    EMatcher.match(Matching, BaseUnmatchedEnums, RevisionUnmatchedEnums);
  }

  // field declaration
  std::vector<std::shared_ptr<SemanticNode>> &BaseUnmatchedFields =
      Matching.PossiblyDeleted[NodeKind::FIELD_DECLARATION];
  std::vector<std::shared_ptr<SemanticNode>> &RevisionUnmatchedFields =
      Matching.PossiblyAdded[NodeKind::FIELD_DECLARATION];
  if (BaseUnmatchedFields.size() && RevisionUnmatchedFields.size()) {
    FieldDeclMatcher FDMatcher;
    FDMatcher.match(Matching, BaseUnmatchedFields, RevisionUnmatchedFields,
                    RefactoredTypes);
  }

  // function definition
  std::vector<std::shared_ptr<SemanticNode>> &BaseUnmatchedFuncDefs =
      Matching.PossiblyDeleted[NodeKind::FUNC_DEF];
  std::vector<std::shared_ptr<SemanticNode>> &RevisionUnmatchedFuncDefs =
      Matching.PossiblyAdded[NodeKind::FUNC_DEF];
  if (BaseUnmatchedFuncDefs.size() && RevisionUnmatchedFuncDefs.size()) {
    FuncDefMatcher FDMatcher;
    FDMatcher.match(Matching, BaseUnmatchedFuncDefs, RevisionUnmatchedFuncDefs,
                    RefactoredTypes);
  }

  // no need to do this for operator cast

  // func special member
  std::vector<std::shared_ptr<SemanticNode>> &BaseUnmatchedFSMembers =
      Matching.PossiblyDeleted[NodeKind::FUNC_SPECIAL_MEMBER];
  std::vector<std::shared_ptr<SemanticNode>> &RevisionUnmatchedFSMembers =
      Matching.PossiblyAdded[NodeKind::FUNC_SPECIAL_MEMBER];
  if (BaseUnmatchedFSMembers.size() && RevisionUnmatchedFSMembers.size()) {
    FuncSpecialMemberMatcher FSMMatcher;
    FSMMatcher.match(Matching, BaseUnmatchedFSMembers,
                     RevisionUnmatchedFSMembers);
  }

  // no need to do this for orphan comment, textual, access specifier
}

TwoWayMatching GraphMatcher::match() {
  auto LamTopDownFunc = [this]() { this->topDownMatch(); };
  auto TopDownElapsed = utils::MeasureRunningTime(LamTopDownFunc);
  spdlog::info("it takes {}ms to do top-down match for side {}", TopDownElapsed,
               magic_enum::enum_name(S));
#ifdef MB_DEBUG
  spdlog::debug("one one matching size: {}", Matching.OneOneMatching.size());
  for (auto &[BaseNode, RevisionNode] : Matching.OneOneMatching) {
    spdlog::debug("kind: {}, matching: {} -> {}",
                  magic_enum::enum_name(BaseNode->getKind()),
                  BaseNode->OriginalSignature, RevisionNode->OriginalSignature);
  }
#endif
  auto LamBottomUpFunc = [this]() { this->bottomUpMatch(); };
  auto BottomUpElapsed = utils::MeasureRunningTime(LamBottomUpFunc);
  spdlog::info("it takes {}ms to do bottom-up match for side {}",
               BottomUpElapsed, magic_enum::enum_name(S));
#ifdef MB_DEBUG
  auto format_unmatched_map =
      [](const std::unordered_map<
          NodeKind, std::vector<std::shared_ptr<SemanticNode>>> &map)
      -> std::string {
    std::ostringstream oss;
    oss << "{\n";
    for (const auto &kv : map) {
      std::ostringstream sigs;
      for (const auto &node : kv.second) {
        sigs << node->OriginalSignature << ", ";
      }
      oss << fmt::format("    {}: [{}], \n", magic_enum::enum_name(kv.first),
                         sigs.str());
    }
    oss << "}";
    return oss.str();
  };
  spdlog::info("possibly deleted: {}",
               format_unmatched_map(Matching.PossiblyDeleted));
  spdlog::info("possibly added: {}",
               format_unmatched_map(Matching.PossiblyAdded));
#endif
  return Matching;
}
} // namespace mergebot::sa
