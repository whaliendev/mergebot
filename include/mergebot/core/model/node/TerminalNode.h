//
// Created by whalien on 08/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TERMINALNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TERMINALNODE_H

#include "mergebot/core/model/SemanticNode.h"
namespace mergebot {
namespace sa {
class TerminalNode : public SemanticNode {

  std::string body() const { return Body; }

  void setBody(std::string const &body) { this->Body = body; }

private:
  std::string Body;
};
} // namespace sa
} // namespace mergebot

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TERMINALNODE_H
