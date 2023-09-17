//
// Created by whalien on 16/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_TYPESPECIFIERMATCHER_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_TYPESPECIFIERMATCHER_H
#include "mergebot/core/model/SemanticNode.h"
#include "mergebot/core/model/mapping/TwoWayMatching.h"
#include "mergebot/core/model/node/TypeDeclNode.h"
#include "mergebot/globals.h"
#include "mergebot/utils/similarity.h"
#include <boost/graph/maximum_weighted_matching.hpp>
#include <vector>
namespace mergebot::sa {
struct TypeSpecifierMatcher {
  //  static constexpr double Thresh = 0.8;
  using TypeDeclGraph =
      boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
                            std::shared_ptr<SemanticNode>,
                            boost::property<boost::edge_weight_t, double>>;
  using vertex_descriptor =
      boost::graph_traits<TypeDeclGraph>::vertex_descriptor;
  using edge_descriptor = boost::graph_traits<TypeDeclGraph>::edge_descriptor;

  void match(TwoWayMatching &Matching,
             std::vector<std::shared_ptr<SemanticNode>> &BaseNodes,
             std::vector<std::shared_ptr<SemanticNode>> &RevisionNodes) {
    TypeDeclGraph TDGraph(BaseNodes.size() + RevisionNodes.size());

    for (size_t i = 0; i < BaseNodes.size(); ++i) {
      TDGraph[i] = BaseNodes[i];
    }
    for (size_t i = 0; i < RevisionNodes.size(); ++i) {
      TDGraph[BaseNodes.size() + i] = RevisionNodes[i];
    }

    for (size_t i = 0; i < BaseNodes.size(); ++i) {
      for (size_t j = 0; j < RevisionNodes.size(); ++j) {
        auto [Edge, Success] = add_edge(i, BaseNodes.size() + j, TDGraph);
        assert(llvm::isa<TypeDeclNode>(BaseNodes[i].get()) &&
               llvm::isa<TypeDeclNode>(RevisionNodes[j].get()));
        put(boost::edge_weight, TDGraph, Edge,
            calcSimilarity(
                llvm::dyn_cast<TypeDeclNode>(BaseNodes[i].get()),
                llvm::dyn_cast<TypeDeclNode>(RevisionNodes[j].get())));
      }
    }

    std::vector<vertex_descriptor> mate(num_vertices(TDGraph));

    boost::maximum_weighted_matching(TDGraph, &mate[0]);

    for (size_t i = 0; i < BaseNodes.size(); ++i) {
      if (mate[i] != boost::graph_traits<TypeDeclGraph>::null_vertex()) {
        std::shared_ptr<SemanticNode> BaseNode = TDGraph[i];
        std::shared_ptr<SemanticNode> RevisionNode = TDGraph[mate[i]];

        auto [Edge, Ok] = boost::edge(i, mate[i], TDGraph);
        if (Ok) {
          auto Weight = get(boost::edge_weight, TDGraph, Edge);
          if (Weight < MIN_SIMI) {
            continue;
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

  double calcSimilarity(const TypeDeclNode *BaseNode,
                        const TypeDeclNode *RevisionNode) {
    // If we add body similarity match, we may predict super type extraction
    size_t IndicatorNum = 0;
    double SimSum = 0;
    auto BaseRefs = BaseNode->References;
    auto RevisionRefs = RevisionNode->References;
    if (available(BaseRefs, RevisionRefs)) {
      IndicatorNum++;
      double SimRef = util::dice(BaseRefs, RevisionRefs);
      if (SimRef < 0) {
        SimRef = 0;
        IndicatorNum--;
      }
      SimSum += SimRef;
    }

    auto BaseNeighbors = BaseNode->getNearestNeighborNames();
    auto RevisionNeighbors = RevisionNode->getNearestNeighborNames();
    if (available(BaseNeighbors, RevisionNeighbors)) {
      IndicatorNum++;
      double SimNeighbors = util::dice(BaseNeighbors, RevisionNeighbors);
      if (SimNeighbors < 0) {
        SimNeighbors = 0;
        IndicatorNum--;
      }
      SimSum += SimNeighbors;
    }

    if (auto BaseParentPtr = BaseNode->Parent.lock()) {
      if (auto RevParentPtr = RevisionNode->Parent.lock()) {
        IndicatorNum++;
        double SimName = util::string_cosine(BaseParentPtr->QualifiedName,
                                             RevParentPtr->QualifiedName);
        if (SimName < 0) {
          SimName = 0;
          IndicatorNum--;
        }
        SimSum += SimName;
      }
    }

    return IndicatorNum ? SimSum / IndicatorNum : 0;
  }
};
} // namespace mergebot::sa

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_TYPESPECIFIERMATCHER_H
