//
// Created by whalien on 12/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_ACCESSSPECIFIERNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_ACCESSSPECIFIERNODE_H

#include "mergebot/core/model/node/TerminalNode.h"

namespace mergebot::sa {
// don't add this node to SemanticGraph
class AccessSpecifierNode : public TerminalNode {
public:
  explicit AccessSpecifierNode(AccessSpecifierKind AccessKind,
                               NodeKind Kind = NodeKind::ACCESS_SPECIFIER)
      : TerminalNode(-1, false, Kind, "", "", "", "", std::nullopt, "", "", 0),
        AccessKind(AccessKind) {
    this->FollowingEOL = 0;
  }

  AccessSpecifierKind AccessKind;

  static bool classof(const SemanticNode *N) {
    return N->getKind() == NodeKind::ACCESS_SPECIFIER;
  }
};
} // namespace mergebot::sa

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_ACCESSSPECIFIERNODE_H
