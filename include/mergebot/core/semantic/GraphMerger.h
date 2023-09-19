//
// Created by whalien on 30/04/23.
//

#ifndef MB_GRAPHMERGER_H
#define MB_GRAPHMERGER_H

#include "mergebot/core/handler/SAHandler.h"
#include "mergebot/core/model/mapping/ThreeWayMapping.h"
#include "mergebot/core/model/mapping/TwoWayMatching.h"
#include "mergebot/core/semantic/GraphBuilder.h"
#include <git2/global.h>

namespace mergebot {
namespace sa {
using SemanticGraph = GraphBuilder::SemanticGraph;
class GraphMerger {
public:
  GraphMerger(const ProjectMeta &Meta, SemanticGraph &OurGraph,
              SemanticGraph &BaseGraph, SemanticGraph &TheirGraph,
              const std::string Dest = "merged")
      : Meta(Meta), MergedDir((fs::path(Meta.MSCacheDir) / Dest).string()),
        OurGraph(OurGraph), BaseGraph(BaseGraph), TheirGraph(TheirGraph) {
    git_libgit2_init();
  }

  ~GraphMerger() { git_libgit2_shutdown(); }

  void threeWayMatch();

  void threeWayMerge();

  std::string getMergedDir() const { return MergedDir; }

private:
  void mergeSemanticNode(std::shared_ptr<SemanticNode> &BaseNode);

  void threeWayMergeChildren(
      const std::vector<std::shared_ptr<SemanticNode>> &OurChildren,
      std::vector<std::shared_ptr<SemanticNode>> &BaseChildren,
      const std::vector<std::shared_ptr<SemanticNode>> &TheirChildren);

  std::vector<std::string>
  mergeStrVecByUnion(const std::vector<std::string> &V1,
                     const std::vector<std::string> &V2,
                     const std::vector<std::string> &V3) const;

  std::string mergeText(const std::string &OurText, const std::string &BaseText,
                        const std::string &TheirText) const;
  std::vector<std::string>
  mergeListTextually(const std::vector<std::string> &OurList,
                     const std::vector<std::string> &BaseList,
                     const std::vector<std::string> &TheirList) const;

  const ProjectMeta &Meta;
  std::string MergedDir;
  SemanticGraph &OurGraph;
  SemanticGraph &BaseGraph;
  SemanticGraph &TheirGraph;
  TwoWayMatching OurMatching;
  TwoWayMatching TheirMatching;
  std::vector<ThreeWayMapping> Mappings;
};

} // namespace sa
} // namespace mergebot

#endif // MB_GRAPHMERGER_H
