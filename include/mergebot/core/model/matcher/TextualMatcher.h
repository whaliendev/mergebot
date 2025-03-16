//
// Created by whalien on 25-3-16.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_TEXTUALMATCHER_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_TEXTUALMATCHER_H

#include "mergebot/core/model/SemanticNode.h"
#include "mergebot/core/model/mapping/TwoWayMatching.h"
#include "mergebot/core/model/node/TextualNode.h"
#include "mergebot/globals.h"
#include "mergebot/utils/similarity.h"
#include <boost/graph/maximum_weighted_matching.hpp>
#include <vector>

namespace mergebot::sa {
class TextualMatcher {
public:
  using TextualGraph =
      boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
                            std::shared_ptr<SemanticNode>,
                            boost::property<boost::edge_weight_t, double>>;
  using vertex_descriptor =
      boost::graph_traits<TextualGraph>::vertex_descriptor;
  using edge_descriptor = boost::graph_traits<TextualGraph>::edge_descriptor;

  void match(TwoWayMatching &Matching,
             std::vector<std::shared_ptr<SemanticNode>> &BaseNodes,
             std::vector<std::shared_ptr<SemanticNode>> &RevisionNodes) {
    TextualGraph TTGraph(BaseNodes.size() + RevisionNodes.size());

    for (size_t i = 0; i < BaseNodes.size(); ++i) {
      TTGraph[i] = BaseNodes[i];
    }
    for (size_t i = 0; i < RevisionNodes.size(); ++i) {
      TTGraph[BaseNodes.size() + i] = RevisionNodes[i];
    }

    for (size_t i = 0; i < BaseNodes.size(); ++i) {
      for (size_t j = 0; j < RevisionNodes.size(); ++j) {
        auto BaseTextualNode = llvm::cast<TextualNode>(BaseNodes[i].get());
        auto RevTextualNode = llvm::cast<TextualNode>(RevisionNodes[j].get());

        if (BaseTextualNode->TUPath != RevTextualNode->TUPath) {
          continue;
        }

        auto [Edge, Success] = add_edge(i, BaseNodes.size() + j, TTGraph);
        assert(llvm::isa<TextualNode>(BaseNodes[i].get()) &&
               llvm::isa<TextualNode>(RevisionNodes[j].get()));
        put(boost::edge_weight, TTGraph, Edge,
            calcSimilarity(
                llvm::dyn_cast<TextualNode>(BaseNodes[i].get()),
                llvm::dyn_cast<TextualNode>(RevisionNodes[j].get())));
      }
    }

    std::vector<vertex_descriptor> mate(num_vertices(TTGraph));

    boost::maximum_weighted_matching(TTGraph, &mate[0]);

    for (size_t i = 0; i < BaseNodes.size(); ++i) {
      if (mate[i] != boost::graph_traits<TextualGraph>::null_vertex()) {
        std::shared_ptr<SemanticNode> BaseNode = TTGraph[i];
        std::shared_ptr<SemanticNode> RevisionNode = TTGraph[mate[i]];

        auto [Edge, Ok] = boost::edge(i, mate[i], TTGraph);
        if (Ok) {
          auto Weight = get(boost::edge_weight, TTGraph, Edge);
          if (Weight < HIGH_SIMI) {
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

  double calcSimilarity(const TextualNode *BaseNode,
                        const TextualNode *RevisionNode) {
    double BodySim = util::string_cosine(BaseNode->Body, RevisionNode->Body);

    std::vector<std::string> BaseNeighbors =
        BaseNode->getNearestNeighborNames();
    std::vector<std::string> RevisionNeighbors =
        RevisionNode->getNearestNeighborNames();
    if (available(BaseNeighbors, RevisionNeighbors)) {
      double NeighborSim = util::dice(BaseNeighbors, RevisionNeighbors);
      return (BodySim + NeighborSim) / 2;
    }

    return BodySim;
  }
};
} // namespace mergebot::sa

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_TEXTUALMATCHER_H
