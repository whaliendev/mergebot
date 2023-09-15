//
// Created by whalien on 15/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_MAPPING_TWOWAYMATCHING_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_MAPPING_TWOWAYMATCHING_H

#include "mergebot/core/model/SemanticNode.h"
#include "mergebot/core/model/enum/NodeKind.h"
#include <boost/bimap.hpp>
#include <unordered_map>
namespace mergebot::sa {
struct TwoWayMatching {
  using BiMap = boost::bimap<std::shared_ptr<SemanticNode>,
                             std::shared_ptr<SemanticNode>>;
  BiMap OneOneMatching;
  std::unordered_map<NodeKind, std::vector<std::shared_ptr<SemanticNode>>>
      PossiblyDeleted; // possibly deleted
  std::unordered_map<NodeKind, std::vector<std::shared_ptr<SemanticNode>>>
      PossiblyAdded; // possibly added

  /// \brief Add a node that is unmatched in the other graph
  /// \param Node Unmatched node
  /// \param IsInBase whether the unmatched node is in base graph
  void addUnmatchedNode(std::shared_ptr<SemanticNode> Node, bool IsInBase) {
    if (IsInBase) {
      PossiblyDeleted[Node->getKind()].push_back(Node);
    } else {
      PossiblyAdded[Node->getKind()].push_back(Node);
    }
  }
};
} // namespace mergebot::sa

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_MAPPING_TWOWAYMATCHING_H
