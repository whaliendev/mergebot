//
// Created by whalien on 30/04/23.
//

#include "mergebot/core/semantic/GraphMatcher.h"

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

void GraphMatcher::bottomUpMatch() {}

TwoWayMatching GraphMatcher::match() {
  topDownMatch();
  bottomUpMatch();
  return Matching;
}
} // namespace mergebot::sa
