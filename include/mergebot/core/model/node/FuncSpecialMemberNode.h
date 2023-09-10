//
// Created by whalien on 10/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_FUNCSPECIALMEMBERNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_FUNCSPECIALMEMBERNODE_H

#include "mergebot/core/model/node/TerminalNode.h"
namespace mergebot {
namespace sa {
// https://en.cppreference.com/w/cpp/language/member_functions#Special_member_functions
/// = default
/// = delete
/// init list
class FuncSpecialMemberNode : public TerminalNode {
public:
  enum DefinitionType {
    Defaulted, // = default
    Deleted,   // = delete
    Plain,     // with body
  };
  FuncSpecialMemberNode(
      int NodeId, bool NeedToMerge, NodeType Type,
      const std::string &DisplayName, const std::string &QualifiedName,
      const std::string &OriginalSignature, std::string &&Comment,
      const std::optional<ts::Point> &Point, std::string &&USR,
      std::string &&Body, size_t FollowingEOL, DefinitionType DefType,
      std::string &&TemplateParameterList, std::string &&Attrs,
      std::string &&BeforeFuncName, std::vector<std::string> &&ParameterList,
      std::vector<std::string> &&InitList)
      : TerminalNode(NodeId, NeedToMerge, Type, DisplayName, QualifiedName,
                     OriginalSignature, std::move(Comment), Point,
                     std::move(USR), std::move(Body), FollowingEOL),
        DefType(DefType),
        TemplateParameterList(std::move(TemplateParameterList)),
        Attrs(std::move(Attrs)), BeforeFuncName(std::move(BeforeFuncName)),
        ParameterList(std::move(ParameterList)), InitList(std::move(InitList)) {
  }

  DefinitionType DefType;
  std::string TemplateParameterList;
  std::string Attrs;
  std::string BeforeFuncName;
  std::vector<std::string> ParameterList;
  std::vector<std::string> InitList;

  std::vector<std::string> ParameterTypes;
};

} // namespace sa
} // namespace mergebot

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_FUNCSPECIALMEMBERNODE_H
