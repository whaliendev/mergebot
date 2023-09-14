//
// Created by whalien on 30/04/23.
//

#ifndef MB_GRAPHMERGER_H
#define MB_GRAPHMERGER_H

#include "mergebot/core/handler/SAHandler.h"
#include "mergebot/core/semantic/GraphBuilder.h"
namespace mergebot {
namespace sa {
using SemanticGraph = GraphBuilder::SemanticGraph;
class GraphMerger {
public:
  GraphMerger(const ProjectMeta &Meta, SemanticGraph &OurGraph,
              SemanticGraph &BaseGraph, SemanticGraph &TheirGraph,
              const std::string Dest = "merged")
      : Meta(Meta), MergedDir((fs::path(Meta.MSCacheDir) / Dest).string()),
        OurGraph(OurGraph), BaseGraph(BaseGraph), TheirGraph(TheirGraph) {}

  void ThreeWayMatch();

  void ThreeWayMerge();

private:
  const ProjectMeta &Meta;
  std::string MergedDir;
  SemanticGraph &OurGraph;
  SemanticGraph &BaseGraph;
  SemanticGraph &TheirGraph;
};

} // namespace sa
} // namespace mergebot

#endif // MB_GRAPHMERGER_H
