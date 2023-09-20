//
// Created by whalien on 30/04/23.
//

#include "mergebot/core/semantic/GraphMerger.h"
#include "mergebot/core/model/node/FieldDeclarationNode.h"
#include "mergebot/core/model/node/FuncDefNode.h"
#include "mergebot/core/model/node/FuncOperatorCastNode.h"
#include "mergebot/core/model/node/FuncSpecialMemberNode.h"
#include "mergebot/core/model/node/OrphanCommentNode.h"
#include "mergebot/core/model/node/TextualNode.h"
#include "mergebot/core/model/node/TranslationUnitNode.h"
#include "mergebot/core/semantic/GraphMatcher.h"
#include "mergebot/core/semantic/graph_export.h"
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
  // meaning less, as the graph info is fixed
  //  ExportGraphToDot(BaseGraph,
  //                   fs::path(Meta.MSCacheDir) / "graphs" / "merged.dot",
  //                   false);
  //  spdlog::info("Merged Graph info:\nvertices: {}\nedges: {}",
  //               boost::num_vertices(BaseGraph), boost::num_edges(BaseGraph));
  spdlog::info("merge completed. ");
}

void GraphMerger::mergeSemanticNode(std::shared_ptr<SemanticNode> &BaseNode) {
  std::shared_ptr<SemanticNode> OurNode =
      OurMatching.OneOneMatching.left.at(BaseNode);
  std::shared_ptr<SemanticNode> TheirNode =
      TheirMatching.OneOneMatching.left.at(BaseNode);
  if (OurNode && TheirNode) {
    if (llvm::isa<TerminalNode>(BaseNode.get())) {
      auto BasePtr = llvm::cast<TerminalNode>(BaseNode.get());
      auto OurPtr = llvm::cast<TerminalNode>(OurNode.get());
      auto TheirPtr = llvm::cast<TerminalNode>(TheirNode.get());
      if (BasePtr && OurPtr && TheirPtr) {
        BasePtr->Body = mergeText(OurPtr->Body, BasePtr->Body, TheirPtr->Body);
      }

      BaseNode->FollowingEOL = TheirNode->FollowingEOL;
      BaseNode->Comment =
          mergeText(OurNode->Comment, BaseNode->Comment, TheirNode->Comment);
      // original signature, do not merge it initially
      BaseNode->QualifiedName =
          mergeText(OurNode->QualifiedName, BaseNode->QualifiedName,
                    TheirNode->QualifiedName);
      // in favor of theirs
      BaseNode->AccessSpecifier = TheirNode->AccessSpecifier;
      if (llvm::isa<FuncDefNode>(BaseNode.get()) ||
          llvm::isa<FuncSpecialMemberNode>(BaseNode.get()) ||
          llvm::isa<FuncOperatorCastNode>(BaseNode.get())) {
        bool SkipSigMerge = false;
        if (BaseNode->OriginalSignature == OurNode->OriginalSignature &&
            BaseNode->OriginalSignature == TheirNode->OriginalSignature) {
          SkipSigMerge = true;
          llvm::cast<TerminalNode>(BaseNode.get())->SigUnchanged = true;
        }
        if (!SkipSigMerge) {
          if (llvm::isa<FuncDefNode>(BaseNode.get())) {
            // 2. func def node, template parameter list, attrs, before func
            // name, displayName, ParameterList, AfterParameterList, body
            auto BaseFuncPtr = llvm::cast<FuncDefNode>(BaseNode.get());
            auto OurFuncPtr = llvm::cast<FuncDefNode>(OurNode.get());
            auto TheirFuncPtr = llvm::cast<FuncDefNode>(TheirNode.get());
            BaseFuncPtr->TemplateParameterList =
                mergeText(OurFuncPtr->TemplateParameterList,
                          BaseFuncPtr->TemplateParameterList,
                          TheirFuncPtr->TemplateParameterList);
            BaseFuncPtr->Attrs = mergeText(
                OurFuncPtr->Attrs, BaseFuncPtr->Attrs, TheirFuncPtr->Attrs);
            BaseFuncPtr->BeforeFuncName = mergeText(
                OurFuncPtr->BeforeFuncName, BaseFuncPtr->BeforeFuncName,
                TheirFuncPtr->BeforeFuncName);
            BaseFuncPtr->ParameterList = mergeListTextually(
                OurFuncPtr->ParameterList, BaseFuncPtr->ParameterList,
                TheirFuncPtr->ParameterList);
            BaseFuncPtr->AfterParameterList = mergeText(
                OurFuncPtr->AfterParameterList, BaseFuncPtr->AfterParameterList,
                TheirFuncPtr->AfterParameterList);
          } else if (llvm::isa<FuncSpecialMemberNode>(BaseNode.get())) {
            // 4. func special member,
            auto BaseFuncPtr =
                llvm::cast<FuncSpecialMemberNode>(BaseNode.get());
            auto OurFuncPtr = llvm::cast<FuncSpecialMemberNode>(OurNode.get());
            auto TheirFuncPtr =
                llvm::cast<FuncSpecialMemberNode>(TheirNode.get());
            BaseFuncPtr->TemplateParameterList =
                mergeText(OurFuncPtr->TemplateParameterList,
                          BaseFuncPtr->TemplateParameterList,
                          TheirFuncPtr->TemplateParameterList);
            if (BaseFuncPtr->Body.size()) {
              BaseFuncPtr->DefType = FuncSpecialMemberNode::Plain;
            } else {
              BaseFuncPtr->DefType =
                  OurFuncPtr->DefType == BaseFuncPtr->DefType
                      ? TheirFuncPtr->DefType
                      : (TheirFuncPtr->DefType == BaseFuncPtr->DefType
                             ? OurFuncPtr->DefType
                             : FuncSpecialMemberNode::Plain);
            }
            BaseFuncPtr->Attrs = mergeText(
                OurFuncPtr->Attrs, BaseFuncPtr->Attrs, TheirFuncPtr->Attrs);
            BaseFuncPtr->BeforeFuncName = mergeText(
                OurFuncPtr->BeforeFuncName, BaseFuncPtr->BeforeFuncName,
                TheirFuncPtr->BeforeFuncName);
            BaseFuncPtr->ParameterList = mergeListTextually(
                OurFuncPtr->ParameterList, BaseFuncPtr->ParameterList,
                TheirFuncPtr->ParameterList);
            BaseFuncPtr->InitList =
                mergeListTextually(OurFuncPtr->InitList, BaseFuncPtr->InitList,
                                   TheirFuncPtr->InitList);
          } else if (llvm::isa<FuncOperatorCastNode>(BaseNode.get())) {
            // 3. func operator cast, the same as func def node
            auto BaseFuncPtr = llvm::cast<FuncOperatorCastNode>(BaseNode.get());
            auto OurFuncPtr = llvm::cast<FuncOperatorCastNode>(OurNode.get());
            auto TheirFuncPtr =
                llvm::cast<FuncOperatorCastNode>(TheirNode.get());
            BaseFuncPtr->TemplateParameterList =
                mergeText(OurFuncPtr->TemplateParameterList,
                          BaseFuncPtr->TemplateParameterList,
                          TheirFuncPtr->TemplateParameterList);
            BaseFuncPtr->Attrs = mergeText(
                OurFuncPtr->Attrs, BaseFuncPtr->Attrs, TheirFuncPtr->Attrs);
            BaseFuncPtr->BeforeFuncName = mergeText(
                OurFuncPtr->BeforeFuncName, BaseFuncPtr->BeforeFuncName,
                TheirFuncPtr->BeforeFuncName);
            BaseFuncPtr->ParameterList = mergeListTextually(
                OurFuncPtr->ParameterList, BaseFuncPtr->ParameterList,
                TheirFuncPtr->ParameterList);
            BaseFuncPtr->AfterParameterList = mergeText(
                OurFuncPtr->AfterParameterList, BaseFuncPtr->AfterParameterList,
                TheirFuncPtr->AfterParameterList);
          }
        }
      }

      BaseNode->OriginalSignature =
          mergeText(OurNode->OriginalSignature, BaseNode->OriginalSignature,
                    TheirNode->OriginalSignature);
    } else { // composite node
      assert(llvm::isa<CompositeNode>(BaseNode.get()));
      BaseNode->FollowingEOL = TheirNode->FollowingEOL;

      // translation unit
      if (llvm::isa<TranslationUnitNode>(BaseNode.get())) [[unlikely]] {
        auto BaseTU = llvm::dyn_cast<TranslationUnitNode>(BaseNode.get());
        auto OurTU = llvm::dyn_cast<TranslationUnitNode>(OurNode.get());
        auto TheirTU = llvm::dyn_cast<TranslationUnitNode>(TheirNode.get());
        if (BaseTU && OurTU && TheirTU) {
          assert(BaseTU->IsHeader == OurTU->IsHeader &&
                 BaseTU->IsHeader == TheirTU->IsHeader &&
                 "only files of the same type can be merged");
          BaseTU->TraditionGuard = TheirTU->TraditionGuard;
          BaseTU->HeaderGuard = TheirTU->HeaderGuard;
          // merge front decls
          BaseTU->FrontDecls = mergeStrVecByUnion(
              OurTU->FrontDecls, BaseTU->FrontDecls, TheirTU->FrontDecls);
        }
      }

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

      assert(llvm::isa<CompositeNode>(TheirNode.get()));
      auto BaseComposite = llvm::cast<CompositeNode>(BaseNode.get());
      auto TheirComposite = llvm::cast<CompositeNode>(TheirNode.get());
      BaseComposite->BeforeFirstChildEOL = TheirComposite->BeforeFirstChildEOL;

      // merge children
      threeWayMergeChildren(OurNode->Children, BaseNode->Children,
                            TheirNode->Children);
    }
  } else {
    // base node exists, any one side doesn't exist, delete it
    BaseNode.reset();
  }
}

std::string GraphMerger::mergeText(const std::string &OurText,
                                   const std::string &BaseText,
                                   const std::string &TheirText) const {
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

void GraphMerger::threeWayMergeChildren(
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
  // TODO(hwa): what if two nodes are from different side however are the same?
  // remove duplicate elements appear in our and their graph,
  // while not appear before in the base graph
  //  std::unordered_set<std::string> Seen;
  //  size_t i = 0;
  //  while (i < BaseChildren.size()) {
  //    auto &Node = BaseChildren[i];
  //    if (Seen.find(Node->QualifiedName) != Seen.end()) {
  //      std::swap(BaseChildren[i], BaseChildren[BaseChildren.size() - 1]);
  //      BaseChildren.pop_back();
  //    } else {
  //      Seen.insert(Node->QualifiedName);
  //      ++i;
  //    }
  //  }
}

std::vector<std::string>
GraphMerger::mergeStrVecByUnion(const std::vector<std::string> &V1,
                                const std::vector<std::string> &V2,
                                const std::vector<std::string> &V3) const {
  std::vector<std::string> Ret;
  Ret.reserve(V1.size() + V2.size() + V3.size());

  Ret = V1;
  std::unordered_map<std::string, int> Fences;
  std::list<std::string> TmpList;
  size_t InsertPos = 0;

  for (size_t i = 0; i < V1.size(); ++i) {
    Fences[V1[i]] = i;
  }

  for (const auto &Vec : {V2, V3}) {
    TmpList.clear();
    for (const auto &item : Vec) {
      if (Fences.find(item) != Fences.end()) {
        if (!TmpList.empty()) {
          Ret.insert(Ret.begin() + InsertPos, TmpList.begin(), TmpList.end());
          TmpList.clear();
        }
        InsertPos = Fences[item] + 1;
      } else {
        TmpList.push_back(item);
      }
    }
    if (!TmpList.empty()) {
      Ret.insert(Ret.end(), TmpList.begin(), TmpList.end());
    }
  }

  std::unordered_set<std::string> Seen;
  Ret.erase(std::remove_if(Ret.begin(), Ret.end(),
                           [&Seen](const auto &Item) {
                             return !Seen.insert(Item).second;
                           }),
            Ret.end());

  return Ret;
}

std::vector<std::string> GraphMerger::mergeListTextually(
    const std::vector<std::string> &OurList,
    const std::vector<std::string> &BaseList,
    const std::vector<std::string> &TheirList) const {
  std::string OurString = util::string_join(OurList, "\n");
  std::string BaseString = util::string_join(BaseList, "\n");
  std::string TheirString = util::string_join(TheirList, "\n");
  std::string MergedString = mergeText(OurString, BaseString, TheirString);
  std::vector<std::string> out;
  std::vector<std::string_view> Splitted =
      util::string_split(MergedString, "\n");
  for (const auto sv : Splitted) {
    out.emplace_back(std::string(sv));
  }
  return out;
}

} // namespace sa
} // namespace mergebot