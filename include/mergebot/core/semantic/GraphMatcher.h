//
// Created by whalien on 30/04/23.
//

#ifndef MB_GRAPHMATCHER_H
#define MB_GRAPHMATCHER_H

#include "mergebot/core/model/mapping/TwoWayMatching.h"
#include "mergebot/core/semantic/GraphBuilder.h"
namespace mergebot::sa {
using SemanticGraph = GraphBuilder::SemanticGraph;

class GraphMatcher {
public:
  TwoWayMatching Matching;

  GraphMatcher(SemanticGraph &BaseGraph, SemanticGraph &RevisionGraph)
      : BaseGraph(BaseGraph), RevisionGraph(RevisionGraph) {}

  TwoWayMatching match();

private:
  void topDownMatch();
  void bottomUpMatch();
  SemanticGraph &BaseGraph;     // parent graph
  SemanticGraph &RevisionGraph; // child graph
};
} // namespace mergebot::sa

#endif // MB_GRAPHMATCHER_H
