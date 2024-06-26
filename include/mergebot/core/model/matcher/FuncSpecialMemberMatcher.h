//
// Created by whalien on 18/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_FUNCSPECIALMEMBERMATCHER_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_FUNCSPECIALMEMBERMATCHER_H
#include "mergebot/core/model/SemanticNode.h"
#include "mergebot/core/model/mapping/TwoWayMatching.h"
#include "mergebot/core/model/node/FuncSpecialMemberNode.h"
#include "mergebot/globals.h"
#include "mergebot/utils/similarity.h"
#include <boost/graph/maximum_weighted_matching.hpp>
#include <vector>
namespace mergebot::sa {
struct FuncSpecialMemberMatcher {
  //  static constexpr double Thresh = 0.8;
  using FuncSpecialGraph =
      boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
                            std::shared_ptr<SemanticNode>,
                            boost::property<boost::edge_weight_t, double>>;
  using vertex_descriptor =
      boost::graph_traits<FuncSpecialGraph>::vertex_descriptor;
  using edge_descriptor =
      boost::graph_traits<FuncSpecialGraph>::edge_descriptor;

  void match(TwoWayMatching &Matching,
             std::vector<std::shared_ptr<SemanticNode>> &BaseNodes,
             std::vector<std::shared_ptr<SemanticNode>> &RevisionNodes) {
    FuncSpecialGraph FSGraph(BaseNodes.size() + RevisionNodes.size());

    for (size_t i = 0; i < BaseNodes.size(); ++i) {
      FSGraph[i] = BaseNodes[i];
    }
    for (size_t i = 0; i < RevisionNodes.size(); ++i) {
      FSGraph[BaseNodes.size() + i] = RevisionNodes[i];
    }

    for (size_t i = 0; i < BaseNodes.size(); ++i) {
      for (size_t j = 0; j < RevisionNodes.size(); ++j) {
        auto [Edge, Success] = add_edge(i, BaseNodes.size() + j, FSGraph);
        assert(llvm::isa<FuncSpecialMemberNode>(BaseNodes[i].get()) &&
               llvm::isa<FuncSpecialMemberNode>(RevisionNodes[j].get()));
        put(boost::edge_weight, FSGraph, Edge,
            calcSimilarity(
                llvm::dyn_cast<FuncSpecialMemberNode>(BaseNodes[i].get()),
                llvm::dyn_cast<FuncSpecialMemberNode>(RevisionNodes[j].get())));
      }
    }

    std::vector<vertex_descriptor> mate(num_vertices(FSGraph));

    boost::maximum_weighted_matching(FSGraph, &mate[0]);

    for (size_t i = 0; i < BaseNodes.size(); ++i) {
      if (mate[i] != boost::graph_traits<FuncSpecialGraph>::null_vertex()) {
        std::shared_ptr<SemanticNode> BaseNode = FSGraph[i];
        std::shared_ptr<SemanticNode> RevisionNode = FSGraph[mate[i]];

        auto [Edge, Ok] = boost::edge(i, mate[i], FSGraph);
        if (Ok) {
          auto Weight = get(boost::edge_weight, FSGraph, Edge);
          if (Weight < MIN_SIMI) {
            continue;
          }

          //          if (auto BaseParentPtr = BaseNode->Parent.lock()) {
          //            if (auto RevParentPtr = RevisionNode->Parent.lock()) {
          //              if (BaseParentPtr->hashSignature() !=
          //                      RevParentPtr->hashSignature() ||
          //                  RefactoredTypes.at(BaseParentPtr->hashSignature())
          //                  !=
          //                      RevParentPtr->hashSignature()) {
          //                continue;
          //                // mark refactoring: field extraction
          //              }
          //            }
          //          }

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

  double calcSimilarity(const FuncSpecialMemberNode *BaseNode,
                        const FuncSpecialMemberNode *RevisionNode) {
    double SimAvg = 0;
    double NameSim = util::string_levenshtein(BaseNode->OriginalSignature,
                                              RevisionNode->OriginalSignature);
    // for terminal node, if body similarity is too low, unnecessary to match
    if (NameSim < MIN_SIMI) {
      return 0;
    }
    SimAvg += NameSim * 0.5;

    double BodySim = util::string_cosine(BaseNode->Body, RevisionNode->Body);
    if (BodySim < 0) {
      BodySim = 0;
    }
    SimAvg += BodySim * 0.2;

    auto BaseInitList = BaseNode->InitList;
    auto RevisionInitList = RevisionNode->InitList;
    if (available(BaseInitList, RevisionInitList)) {
      double InitListSim = util::dice(BaseInitList, RevisionInitList);
      if (InitListSim < 0) {
        InitListSim = 0;
      }
      SimAvg += InitListSim * 0.1;
    }

    // following 4 are contexts
    // USE
    auto BaseParamTypes = BaseNode->ParameterTypes;
    auto RevisionParamTypes = RevisionNode->ParameterTypes;
    if (available(BaseParamTypes, RevisionParamTypes)) {
      double ParamTypeSim = util::dice(BaseParamTypes, RevisionParamTypes);
      if (ParamTypeSim < 0) {
        ParamTypeSim = 0;
      }
      SimAvg += ParamTypeSim * 0.05;
    }

    // REFS
    auto BaseRefs = BaseNode->References;
    auto RevisionRefs = RevisionNode->References;
    if (available(BaseRefs, RevisionRefs)) {
      double RefSim = util::dice(BaseRefs, RevisionRefs);
      if (RefSim < 0) {
        RefSim = 0;
      }
      SimAvg += RefSim * 0.05;
    }

    // NEIGHBORS
    auto BaseNeighbors = BaseNode->getNearestNeighborNames();
    auto RevisionNeighbors = RevisionNode->getNearestNeighborNames();
    if (available(BaseNeighbors, RevisionNeighbors)) {
      double SimNeighbors = util::dice(BaseNeighbors, RevisionNeighbors);
      if (SimNeighbors < 0) {
        SimNeighbors = 0;
      }
      SimAvg += SimNeighbors * 0.05;
    }

    // CONTAINS
    if (auto BaseParentPtr = BaseNode->Parent.lock()) {
      if (auto RevParentPtr = RevisionNode->Parent.lock()) {
        double SimName = util::string_levenshtein(BaseParentPtr->QualifiedName,
                                                  RevParentPtr->QualifiedName);
        if (SimName < 0) {
          SimName = 0;
        }
        SimAvg += SimName * 0.05;
      }
    }

    return SimAvg;
  }
};
} // namespace mergebot::sa
#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_FUNCSPECIALMEMBERMATCHER_H
