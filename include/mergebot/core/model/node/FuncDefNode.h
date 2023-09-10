//
// Created by whalien on 10/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_FUNCDEFNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_FUNCDEFNODE_H

#include "mergebot/core/model/node/TerminalNode.h"
namespace mergebot {
namespace sa {
/// trailing return type or plain function
class FuncDefNode : public TerminalNode {
public:
  FuncDefNode(int NodeId, bool NeedToMerge, NodeType Type,
              const std::string &DisplayName, const std::string &QualifiedName,
              const std::string &OriginalSignature, std::string &&Comment,
              const std::optional<ts::Point> &Point, std::string &&USR,
              std::string &&Body, size_t FollowingEOL,
              std::string &&TemplateParameterList, std::string &&Attrs,
              std::string &&BeforeFuncName,
              std::vector<std::string> &&ParameterList,
              std::string &&AfterParameterList)
      : TerminalNode(NodeId, NeedToMerge, Type, DisplayName, QualifiedName,
                     OriginalSignature, std::move(Comment), Point,
                     std::move(USR), std::move(Body), FollowingEOL),
        TemplateParameterList(std::move(TemplateParameterList)),
        Attrs(std::move(Attrs)), BeforeFuncName(std::move(BeforeFuncName)),
        ParameterList(std::move(ParameterList)),
        AfterParameterList(std::move(AfterParameterList)) {}

  std::string TemplateParameterList;
  std::string Attrs;
  std::string BeforeFuncName; // return value, virtual, storage specifier, etc.
  std::vector<std::string> ParameterList;
  std::string AfterParameterList;

  // extract from USR for USE edge
  std::vector<std::string> ParameterTypes;
};

} // namespace sa
} // namespace mergebot

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_FUNCDEFNODE_H
