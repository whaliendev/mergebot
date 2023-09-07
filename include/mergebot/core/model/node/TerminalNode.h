//
// Created by whalien on 08/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TERMINALNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TERMINALNODE_H

#include "mergebot/core/model/SemanticNode.h"
namespace mergebot {
namespace sa {
class TerminalNode : public SemanticNode {
public:
  TerminalNode(int NodeId, bool NeedToMerge, NodeType Type,
               const std::string &DisplayName, const std::string &QualifiedName,
               const std::string &OriginalSignature, std::string &&Comment,
               const std::optional<ts::Point> &Point, std::string &&USR,
               std::string &&Body)
      : SemanticNode(NodeId, NeedToMerge, Type, DisplayName, QualifiedName,
                     OriginalSignature, std::move(Comment), Point,
                     std::move(USR)),
        Body(std::move(Body)) {}

  friend bool operator==(TerminalNode const &lhs, TerminalNode const &rhs) {
    return lhs.Body == rhs.Body;
  }

  std::string body() const { return Body; }

  void setBody(std::string const &body) { this->Body = body; }

  std::string Body;
};
} // namespace sa
} // namespace mergebot

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TERMINALNODE_H
