//
// Created by whalien on 23/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_NAMESPACEMATCHER_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_NAMESPACEMATCHER_H
#include "mergebot/core/model/SemanticNode.h"
#include "mergebot/core/model/mapping/TwoWayMatching.h"
#include "mergebot/core/model/node/NamespaceNode.h"
#include "mergebot/globals.h"
#include "mergebot/utils/similarity.h"
#include <boost/graph/maximum_weighted_matching.hpp>
#include <vector>

namespace mergebot::sa {
class NamespaceMatcher {
public:
  using NamespaceGraph =
      boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
                            std::shared_ptr<SemanticNode>,
                            boost::property<boost::edge_weight_t, double>>;
  using vertex_descriptor =
      boost::graph_traits<NamespaceGraph>::vertex_descriptor;
  using edge_descriptor = boost::graph_traits<NamespaceGraph>::edge_descriptor;

  void match(TwoWayMatching &Matching,
             std::vector<std::shared_ptr<SemanticNode>> &BaseNodes,
             std::vector<std::shared_ptr<SemanticNode>> &RevisionNodes) {
    NamespaceGraph NSGraph(BaseNodes.size() + RevisionNodes.size());

    for (size_t i = 0; i < BaseNodes.size(); ++i) {
      NSGraph[i] = BaseNodes[i];
    }
    for (size_t i = 0; i < RevisionNodes.size(); ++i) {
      NSGraph[BaseNodes.size() + i] = RevisionNodes[i];
    }

    for (size_t i = 0; i < BaseNodes.size(); ++i) {
      for (size_t j = 0; j < RevisionNodes.size(); ++j) {
        auto BaseNSNode = llvm::cast<NamespaceNode>(BaseNodes[i].get());
        auto RevNSNode = llvm::cast<NamespaceNode>(RevisionNodes[j].get());
        if (BaseNSNode->TUPath != RevNSNode->TUPath) {
          continue;
        }

        auto [Edge, Success] = add_edge(i, BaseNodes.size() + j, NSGraph);
        assert(llvm::isa<NamespaceNode>(BaseNodes[i].get()) &&
               llvm::isa<NamespaceNode>(RevisionNodes[j].get()));
        put(boost::edge_weight, NSGraph, Edge,
            calcSimilarity(
                llvm::dyn_cast<NamespaceNode>(BaseNodes[i].get()),
                llvm::dyn_cast<NamespaceNode>(RevisionNodes[j].get())));
      }
    }

    std::vector<vertex_descriptor> mate(num_vertices(NSGraph));

    boost::maximum_weighted_matching(NSGraph, &mate[0]);

    for (size_t i = 0; i < BaseNodes.size(); ++i) {
      if (mate[i] != boost::graph_traits<NamespaceGraph>::null_vertex()) {
        std::shared_ptr<SemanticNode> BaseNode = NSGraph[i];
        std::shared_ptr<SemanticNode> RevisionNode = NSGraph[mate[i]];

        auto [Edge, Ok] = boost::edge(i, mate[i], NSGraph);
        if (Ok) {
          auto Weight = get(boost::edge_weight, NSGraph, Edge);
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

  double calcSimilarity(const NamespaceNode *BaseNode,
                        const NamespaceNode *RevisionNode) {
    double SigSim = util::string_cosine(BaseNode->OriginalSignature,
                                        RevisionNode->OriginalSignature);

    double NameSim = util::string_cosine(BaseNode->QualifiedName,
                                         RevisionNode->QualifiedName);

    std::vector<std::string> BaseNeighbors =
        BaseNode->getNearestNeighborNames();
    std::vector<std::string> RevisionNeighbors =
        RevisionNode->getNearestNeighborNames();
    if (available(BaseNeighbors, RevisionNeighbors)) {
      double NeighborSim = util::dice(BaseNeighbors, RevisionNeighbors);
      return (SigSim + NameSim + NeighborSim) / 3;
    }

    return (SigSim + NameSim) / 2;
  }
};
} // namespace mergebot::sa

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_MATCHER_NAMESPACEMATCHER_H
