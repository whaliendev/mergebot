//
// Created by whalien on 30/04/23.
//

#include "mergebot/core/semantic/GraphMerger.h"
#include "mergebot/core/model/node/OrphanCommentNode.h"
#include "mergebot/core/model/node/TranslationUnitNode.h"
#include "mergebot/core/semantic/GraphMatcher.h"
#include "mergebot/core/semantic/pretty_printer.h"
#include "mergebot/utils/gitservice.h"
#include <oneapi/tbb/parallel_invoke.h>

namespace mergebot {
namespace sa {

void GraphMerger::threeWayMatch() {
  GraphMatcher OurMatcher(BaseGraph, OurGraph);
  GraphMatcher TheirMatcher(BaseGraph, TheirGraph);
  tbb::parallel_invoke([&]() { OurMatching = OurMatcher.match(); },
                       [&]() { TheirMatching = TheirMatcher.match(); });

  std::unordered_set<std::shared_ptr<SemanticNode>> NeedToMergeNodes;
  for (auto VD : boost::make_iterator_range(boost::vertices(BaseGraph))) {
    auto &Node = BaseGraph[VD];
    if (Node->NeedToMerge || !Node->IsSynthetic) {
      NeedToMergeNodes.insert(Node);
    }
  }

  for (auto &NodePtr : NeedToMergeNodes) {
    if (llvm::isa<TranslationUnitNode>(NodePtr.get()) && NodePtr->NeedToMerge) {
      ThreeWayMapping mapping(OurMatching.OneOneMatching.left.at(NodePtr),
                              NodePtr,
                              TheirMatching.OneOneMatching.left.at(NodePtr));
      Mappings.emplace_back(std::move(mapping));
    }
  }
  spdlog::info("three way match done. Base graph vertices num: {}, OurMatching "
               "size: {}, TheirMatching size: {}",
               boost::num_vertices(BaseGraph),
               OurMatching.OneOneMatching.size(),
               TheirMatching.OneOneMatching.size());
}

void GraphMerger::threeWayMerge() {
  for (const auto &Mapping : Mappings) {
    assert(Mapping.BaseNode.has_value() &&
           llvm::isa<TranslationUnitNode>(Mapping.BaseNode.value().get()));
    std::shared_ptr<SemanticNode> BaseNodePtr = Mapping.BaseNode.value();
    mergeSemanticNode(BaseNodePtr);
    if (BaseNodePtr) {
      PrettyPrintTU(BaseNodePtr, MergedDir);
    }
  }
}

void GraphMerger::mergeSemanticNode(std::shared_ptr<SemanticNode> &BaseNode) {
  std::shared_ptr<SemanticNode> OurNode =
      OurMatching.OneOneMatching.left.at(BaseNode);
  std::shared_ptr<SemanticNode> TheirNode =
      TheirMatching.OneOneMatching.left.at(BaseNode);
  if (OurNode && TheirNode) {
    if (llvm::isa<TerminalNode>(BaseNode.get())) {
      if (auto OrphanCommentRawPtr =
              llvm::dyn_cast<OrphanCommentNode>(BaseNode.get())) [[unlikely]] {
        // when print comment, print its QualifiedName
        OrphanCommentRawPtr->Body = mergeText(
            OurNode->QualifiedName, OrphanCommentRawPtr->QualifiedName,
            TheirNode->QualifiedName);
      } else {
        // 1. field declaration node, merge its textual content;
        // 2. func def node, template parameter list, attrs, before func
        // name, displayName, ParameterList, AfterParameterList, body
        // 3. func operator cast, the same as func def node
        // 4. func special member,
        // 5. orphan comment
        // 6. textual
      }
    } else { // composite node
      assert(llvm::isa<CompositeNode>(BaseNode.get()));

      std::string MergedComment =
          mergeText(OurNode->Comment, BaseNode->Comment, TheirNode->Comment);
      std::string MergedSignature =
          mergeText(OurNode->OriginalSignature, BaseNode->OriginalSignature,
                    TheirNode->OriginalSignature);
      BaseNode->Comment = std::move(MergedComment);
      BaseNode->OriginalSignature = std::move(MergedSignature);
      BaseNode->QualifiedName =
          mergeText(OurNode->QualifiedName, BaseNode->QualifiedName,
                    TheirNode->QualifiedName);
      // in favor of their side
      BaseNode->FollowingEOL = TheirNode->FollowingEOL;

      // merge children
      mergeChildrenByUnion(OurNode->Children, BaseNode->Children,
                           TheirNode->Children);
    }
  } else {
    // base node exists, any one side doesn't exist, delete it
    BaseNode.reset();
  }
}

std::string GraphMerger::mergeText(const std::string &OurText,
                                   const std::string &BaseText,
                                   const std::string &TheirText) {
  if (OurText == BaseText) {
    return TheirText;
  }
  if (TheirText == BaseText) {
    return OurText;
  }
  std::string MergedText = util::git_merge_textual(
      OurText, BaseText, TheirText, Meta.MS.base, Meta.MS.theirs);
  return MergedText;
}

void GraphMerger::mergeChildrenByUnion(
    const std::vector<std::shared_ptr<SemanticNode>> &OurChildren,
    std::vector<std::shared_ptr<SemanticNode>> &BaseChildren,
    const std::vector<std::shared_ptr<SemanticNode>> &TheirChildren) {

  std::vector<size_t> Fences;
  int FenceIdx = -1;
  size_t InsertPos = 0;

  // 处理 BaseChildren 并生成 Fences
  for (size_t i = 0; i < BaseChildren.size(); ++i) {
    auto &BaseChild = BaseChildren[i];
    if (OurMatching.OneOneMatching.left.find(BaseChild) !=
            OurMatching.OneOneMatching.left.end() &&
        TheirMatching.OneOneMatching.left.find(BaseChild) !=
            TheirMatching.OneOneMatching.left.end()) {
      Fences.emplace_back(i);
      mergeSemanticNode(BaseChild);
    }
  }

  // 处理 OurChildren
  for (const auto &OurChild : OurChildren) {
    if (FenceIdx < 0) {
      FenceIdx = FenceIdx + 1 >= 0 ? FenceIdx : -1;
      InsertPos = Fences[FenceIdx + 1];
    } else if (FenceIdx >= static_cast<int>(Fences.size() - 1)) {
      InsertPos = BaseChildren.size();
    } else {
      InsertPos = Fences[FenceIdx + 1];
    }

    if (OurMatching.OneOneMatching.right.find(OurChild) !=
        OurMatching.OneOneMatching.right.end()) {
      auto &BaseNode = OurMatching.OneOneMatching.right.at(OurChild);
      if (TheirMatching.OneOneMatching.left.find(BaseNode) !=
          TheirMatching.OneOneMatching.left.end()) {
        // fences
        FenceIdx++;
      }
      // skip if only appear in one side, as it has been handled
    } else {
      // insert
      BaseChildren.insert(BaseChildren.begin() + InsertPos, OurChild);
      for (size_t i = FenceIdx + 1; i < Fences.size(); ++i) {
        Fences[i]++;
      }
    }
  }

  FenceIdx = -1;
  // 处理 TheirChildren
  for (const auto &TheirChild : TheirChildren) {
    if (FenceIdx == -1) {
      FenceIdx = FenceIdx + 1 >= 0 ? FenceIdx : -1;
      InsertPos = Fences[FenceIdx + 1];
    } else if (FenceIdx >= static_cast<int>(Fences.size() - 1)) {
      InsertPos = BaseChildren.size();
    } else {
      InsertPos = Fences[FenceIdx + 1];
    }

    if (TheirMatching.OneOneMatching.right.find(TheirChild) !=
        TheirMatching.OneOneMatching.right.end()) {
      auto &BaseNode = TheirMatching.OneOneMatching.right.at(TheirChild);
      if (OurMatching.OneOneMatching.left.find(BaseNode) !=
          OurMatching.OneOneMatching.left.end()) {
        // fences
        FenceIdx++;
      }
      // skip if only appear in one side, as it has been handled
    } else {
      // insert
      BaseChildren.insert(BaseChildren.begin() + InsertPos, TheirChild);
      for (size_t i = FenceIdx + 1; i < Fences.size(); ++i) {
        Fences[i]++;
      }
    }
  }

  // remove nullptr
  BaseChildren.erase(std::remove_if(BaseChildren.begin(), BaseChildren.end(),
                                    [](const auto &Node) { return !Node; }),
                     BaseChildren.end());
  // remove duplicate elements appear in our and their graph,
  // while not appear before in the base graph
  std::unordered_set<std::string> Seen;
  size_t i = 0;
  while (i < BaseChildren.size()) {
    auto &Node = BaseChildren[i];
    if (Seen.find(Node->QualifiedName) != Seen.end()) {
      std::swap(BaseChildren[i], BaseChildren[BaseChildren.size() - 1]);
      BaseChildren.pop_back();
    } else {
      Seen.insert(Node->QualifiedName);
      ++i;
    }
  }
}

} // namespace sa
} // namespace mergebot