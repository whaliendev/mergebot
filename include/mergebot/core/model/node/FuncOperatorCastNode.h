//
// Created by whalien on 10/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_FUNCOPERATORCASTNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_FUNCOPERATORCASTNODE_H

#include "mergebot/core/model/node/TerminalNode.h"
namespace mergebot::sa {
/// @code
/// template <typename U, typename T>
/// operator int() const {
///   return static_cast<int>(static_cast<U>(static_cast<T>(0)));
/// }
/// @endcodes
class FuncOperatorCastNode : public TerminalNode {
public:
  FuncOperatorCastNode(int NodeId, bool NeedToMerge, NodeKind Kind,
                       const std::string &DisplayName,
                       const std::string &QualifiedName,
                       const std::string &OriginalSignature,
                       std::string &&Comment,
                       const std::optional<ts::Point> &Point, std::string &&USR,
                       std::string &&Body, size_t FollowingEOL,
                       std::string &&TemplateParameterList, std::string &&Attrs,
                       std::vector<std::string> &&ParameterList,
                       std::string &&AfterParameterList)
      : TerminalNode(NodeId, NeedToMerge, Kind, DisplayName, QualifiedName,
                     OriginalSignature, std::move(Comment), Point,
                     std::move(USR), std::move(Body), FollowingEOL),
        TemplateParameterList(std::move(TemplateParameterList)),
        Attrs(std::move(Attrs)), ParameterList(std::move(ParameterList)),
        AfterParameterList(std::move(AfterParameterList)) {}

  static bool classof(const SemanticNode *N) {
    return N->getKind() == NodeKind::FUNC_OPERATOR_CAST;
  }

  std::string TemplateParameterList;
  std::string Attrs;
  std::string BeforeFuncName; // return value, virtual, storage specifier, etc.
  std::vector<std::string> ParameterList;
  std::string AfterParameterList;
};
} // namespace mergebot::sa

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_FUNCOPERATORCASTNODE_H
