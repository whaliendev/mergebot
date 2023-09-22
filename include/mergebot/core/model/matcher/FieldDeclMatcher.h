//
// Created by whalien on 17/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_FIELDDECLMATCHER_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_FIELDDECLMATCHER_H
#include "mergebot/core/model/SemanticNode.h"
#include "mergebot/core/model/mapping/TwoWayMatching.h"
#include "mergebot/core/model/node/FieldDeclarationNode.h"
#include "mergebot/globals.h"
#include "mergebot/utils/similarity.h"
#include <boost/graph/maximum_weighted_matching.hpp>
#include <vector>
namespace mergebot::sa {
struct FieldDeclMatcher {
  using FieldDeclGraph =
      boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
                            std::shared_ptr<SemanticNode>,
                            boost::property<boost::edge_weight_t, double>>;
  using vertex_descriptor =
      boost::graph_traits<FieldDeclGraph>::vertex_descriptor;
  using edge_descriptor = boost::graph_traits<FieldDeclGraph>::edge_descriptor;

  void match(TwoWayMatching &Matching,
             std::vector<std::shared_ptr<SemanticNode>> &BaseNodes,
             std::vector<std::shared_ptr<SemanticNode>> &RevisionNodes,
             const std::unordered_map<size_t, size_t> &RefactoredTypes) {
    FieldDeclGraph FDGraph(BaseNodes.size() + RevisionNodes.size());

    for (size_t i = 0; i < BaseNodes.size(); ++i) {
      FDGraph[i] = BaseNodes[i];
    }
    for (size_t i = 0; i < RevisionNodes.size(); ++i) {
      FDGraph[BaseNodes.size() + i] = RevisionNodes[i];
    }

    for (size_t i = 0; i < BaseNodes.size(); ++i) {
      for (size_t j = 0; j < RevisionNodes.size(); ++j) {
        auto BaseFieldDecl =
            llvm::cast<FieldDeclarationNode>(BaseNodes[i].get());
        auto RevFieldDecl =
            llvm::cast<FieldDeclarationNode>(RevisionNodes[j].get());
        double DeclaratorSim = util::string_cosine(BaseFieldDecl->Declarator,
                                                   RevFieldDecl->Declarator);
        if (DeclaratorSim < MIN_SIMI) {
          continue;
        }

        auto [Edge, Success] = add_edge(i, BaseNodes.size() + j, FDGraph);
        assert(llvm::isa<FieldDeclarationNode>(BaseNodes[i].get()) &&
               llvm::isa<FieldDeclarationNode>(RevisionNodes[j].get()));
        put(boost::edge_weight, FDGraph, Edge,
            calcSimilarity(
                llvm::dyn_cast<FieldDeclarationNode>(BaseNodes[i].get()),
                llvm::dyn_cast<FieldDeclarationNode>(RevisionNodes[j].get())));
      }
    }

    std::vector<vertex_descriptor> mate(num_vertices(FDGraph));

    boost::maximum_weighted_matching(FDGraph, &mate[0]);

    for (size_t i = 0; i < BaseNodes.size(); ++i) {
      if (mate[i] != boost::graph_traits<FieldDeclGraph>::null_vertex()) {
        std::shared_ptr<SemanticNode> BaseNode = FDGraph[i];
        std::shared_ptr<SemanticNode> RevisionNode = FDGraph[mate[i]];

        auto [Edge, Ok] = boost::edge(i, mate[i], FDGraph);
        if (Ok) {
          auto Weight = get(boost::edge_weight, FDGraph, Edge);
          if (Weight < MIN_SIMI) {
            continue;
          }

          if (auto BaseParentPtr = BaseNode->Parent.lock()) {
            if (auto RevParentPtr = RevisionNode->Parent.lock()) {
              if (BaseParentPtr->hashSignature() !=
                      RevParentPtr->hashSignature() ||
                  RefactoredTypes.find(BaseParentPtr->hashSignature()) ==
                      RefactoredTypes.end() ||
                  RefactoredTypes.at(BaseParentPtr->hashSignature()) !=
                      RevParentPtr->hashSignature()) {
                continue;
                // mark refactoring: field extraction
              }
            }
          }

          //          spdlog::debug("refactor: {}({}) -> {}({})",
          //                        BaseNode->OriginalSignature,
          //                        magic_enum::enum_name(BaseNode->getKind()),
          //                        RevisionNode->OriginalSignature,
          //                        magic_enum::enum_name(RevisionNode->getKind()));
          Matching.OneOneMatching.insert({BaseNode, RevisionNode});

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
  bool available(const std::vector<std::string> &BaseRefs,
                 const std::vector<std::string> &RevisionRefs) {
    if (BaseRefs.empty() || RevisionRefs.empty()) {
      return false;
    }
    bool BaseAllEmpty =
        std::all_of(BaseRefs.begin(), BaseRefs.end(),
                    [&](const auto &item) { return item.empty(); });
    bool RevisionAllEmpty =
        std::all_of(RevisionRefs.begin(), RevisionRefs.end(),
                    [&](const auto &item) { return item.empty(); });
    return !(BaseAllEmpty || RevisionAllEmpty);
  }

  double calcSimilarity(const FieldDeclarationNode *BaseNode,
                        const FieldDeclarationNode *RevisionNode) {
    double SimAvg = 0;

    if (auto BaseParentPtr = BaseNode->Parent.lock()) {
      if (auto RevParentPtr = RevisionNode->Parent.lock()) {
        double SimName = util::string_levenshtein(BaseParentPtr->QualifiedName,
                                                  RevParentPtr->QualifiedName) *
                         0.3;
        if (SimName < 0) {
          SimName = 0;
        }
        SimAvg += SimName;
      }
    }

    if (!BaseNode->Declarator.empty() && !RevisionNode->Declarator.empty()) {
      double SimDeclarator =
          util::string_cosine(BaseNode->Declarator, RevisionNode->Declarator) *
          0.2;
      if (SimDeclarator < MIN_SIMI * 0.2) {
        return 0;
      }
      SimAvg += SimDeclarator;
    }

    auto BaseRefs = BaseNode->References;
    auto RevisionRefs = RevisionNode->References;
    if (available(BaseRefs, RevisionRefs)) {
      double SimRef = util::dice(BaseRefs, RevisionRefs) * 0.2;
      if (SimRef < 0) {
        SimRef = 0;
      }
      SimAvg += SimRef;
    }

    auto BaseNeighbors = BaseNode->getNearestNeighborNames();
    auto RevisionNeighbors = RevisionNode->getNearestNeighborNames();
    if (available(BaseNeighbors, RevisionNeighbors)) {
      double SimNeighbors = util::dice(BaseNeighbors, RevisionNeighbors) * 0.3;
      if (SimNeighbors < 0) {
        SimNeighbors = 0;
      }
      SimAvg += SimNeighbors;
    }

    return SimAvg;
  }
};
} // namespace mergebot::sa
#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_FIELDDECLMATCHER_H
