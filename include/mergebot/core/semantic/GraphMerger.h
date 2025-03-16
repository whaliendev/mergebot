//
// Created by whalien on 30/04/23.
//

#ifndef MB_GRAPHMERGER_H
#define MB_GRAPHMERGER_H

#include "mergebot/core/handler/SAHandler.h"
#include "mergebot/core/model/mapping/ThreeWayMapping.h"
#include "mergebot/core/model/mapping/TwoWayMatching.h"
#include "mergebot/core/semantic/GraphBuilder.h"
#include <cstdint>
#include <git2/global.h>

namespace mergebot {
namespace sa {
using SemanticGraph = GraphBuilder::SemanticGraph;
class GraphMerger {
public:
  GraphMerger(const ProjectMeta &Meta, SemanticGraph &OurGraph,
              SemanticGraph &BaseGraph, SemanticGraph &TheirGraph,
              const std::string &OurSideId, const std::string &BaseSideId,
              const std::string &TheirSideId, const std::string Dest = "merged")
      : Meta(Meta), MergedDir((fs::path(Meta.MSCacheDir) / Dest).string()),
        OurGraph(OurGraph), BaseGraph(BaseGraph), TheirGraph(TheirGraph),
        OurSideId(OurSideId), BaseSideId(BaseSideId), TheirSideId(TheirSideId) {
    git_libgit2_init();
    initOrderInFavour();
  }

  ~GraphMerger() { git_libgit2_shutdown(); }

  void threeWayMatch();

  std::vector<std::string> threeWayMerge();

  std::string getMergedDir() const { return MergedDir; }

private:
  using RCSemanticNode = std::shared_ptr<SemanticNode>;
  using NeighborTuple =
      std::tuple<RCSemanticNode, RCSemanticNode, RCSemanticNode>;
  void mergeSemanticNode(std::shared_ptr<SemanticNode> &BaseNode);

  void threeWayMergeChildren(
      const std::vector<std::shared_ptr<SemanticNode>> &OurChildren,
      std::vector<std::shared_ptr<SemanticNode>> &BaseChildren,
      const std::vector<std::shared_ptr<SemanticNode>> &TheirChildren);

  std::vector<std::shared_ptr<SemanticNode>>
  directMergeChildren(const std::shared_ptr<SemanticNode> &OurNode,
                      const std::shared_ptr<SemanticNode> &BaseNode,
                      const std::shared_ptr<SemanticNode> &TheirNode);

  std::vector<std::shared_ptr<SemanticNode>>
  directMergeChildrenInOurOrder(const std::shared_ptr<SemanticNode> &OurNode,
                                const std::shared_ptr<SemanticNode> &BaseNode,
                                const std::shared_ptr<SemanticNode> &TheirNode);

  /// Merge children nodes from three versions (base, ours, theirs),
  /// prioritizing the order from our version
  /// @param OurNode The node from our version
  /// @param BaseNode The node from base version
  /// @param TheirNode The node from their version
  /// @return A vector of merged children nodes
  std::vector<std::shared_ptr<SemanticNode>> directMergeChildrenInTheirOrder(
      const std::shared_ptr<SemanticNode> &OurNode,
      const std::shared_ptr<SemanticNode> &BaseNode,
      const std::shared_ptr<SemanticNode> &TheirNode);

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

  std::vector<std::shared_ptr<SemanticNode>>
  filterAddedNodes(const std::shared_ptr<SemanticNode> &BaseNode,
                   const TwoWayMatching &Matching) const;

  std::vector<std::shared_ptr<SemanticNode>> removeDuplicates(
      std::vector<std::shared_ptr<SemanticNode>> &&OurAdded,
      std::vector<std::shared_ptr<SemanticNode>> &&TheirAdded) const;

  std::optional<NeighborTuple> getNeighbors(const RCSemanticNode &Node) const;

  void initOrderInFavour();

  const ProjectMeta &Meta;
  std::string MergedDir;
  SemanticGraph &OurGraph;
  SemanticGraph &BaseGraph;
  SemanticGraph &TheirGraph;

  std::string OurSideId;
  std::string BaseSideId;
  std::string TheirSideId;

  TwoWayMatching OurMatching;
  TwoWayMatching TheirMatching;
  std::vector<ThreeWayMapping> Mappings;

  enum class OrderInfavour : uint8_t { Ours, Theirs };
  OrderInfavour OrderInFavour;
};

} // namespace sa
} // namespace mergebot

#endif // MB_GRAPHMERGER_H
