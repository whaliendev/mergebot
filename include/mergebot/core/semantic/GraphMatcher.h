//
// Created by whalien on 30/04/23.
//

#ifndef MB_GRAPHMATCHER_H
#define MB_GRAPHMATCHER_H

#include "mergebot/core/model/enum/Side.h"
#include "mergebot/core/model/mapping/TwoWayMatching.h"
#include "mergebot/core/semantic/GraphBuilder.h"
namespace mergebot::sa {
using SemanticGraph = GraphBuilder::SemanticGraph;

class GraphMatcher {
public:
  TwoWayMatching Matching;

  GraphMatcher(SemanticGraph &BaseGraph, SemanticGraph &RevisionGraph, Side S)
      : BaseGraph(BaseGraph), RevisionGraph(RevisionGraph), S(S) {}

  TwoWayMatching match();

private:
  void topDownMatch();
  void bottomUpMatch();
  SemanticGraph &BaseGraph;     // parent graph
  SemanticGraph &RevisionGraph; // child graph

  Side S;
};
} // namespace mergebot::sa

#endif // MB_GRAPHMATCHER_H
