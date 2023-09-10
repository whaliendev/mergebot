//
// Created by whalien on 03/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TYPEDECLNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TYPEDECLNODE_H

#include "mergebot/core/model/node/CompositeNode.h"
#include <stack>

namespace mergebot::sa {
// class, struct, union
class TypeDeclNode : public CompositeNode {
public:
  enum class TypeDeclKind : uint8_t {
    Class,
    Struct,
    Union,
  };

  TypeDeclNode(int NodeId, bool NeedToMerge, NodeType Type,
               const std::string &DisplayName, const std::string &QualifiedName,
               const std::string &OriginalSignature, std::string &&Comment,
               const std::optional<ts::Point> &Point, std::string &&USR,
               size_t BeforeFirstChildEOL, TypeDeclKind Kind,
               std::string &&Attrs, bool IsFinal, std::string &&BaseClause,
               AccessSpecifierKind FirstAccessSpecifier,
               std::string &&TemplateParameterList)
      : CompositeNode(NodeId, NeedToMerge, Type, DisplayName, QualifiedName,
                      OriginalSignature, std::move(Comment), Point,
                      std::move(USR), BeforeFirstChildEOL),
        Kind(Kind), Attrs(std::move(Attrs)), IsFinal(IsFinal),
        BaseClause(std::move(BaseClause)),
        TemplateParameterList(std::move(TemplateParameterList)) {
    this->AccessSpecifierStack.push(std::make_pair(FirstAccessSpecifier, 0));
  }

private:
  TypeDeclKind Kind;
  std::string Attrs;
  bool IsFinal;
  std::string BaseClause;
  //  AccessSpecifierKind FirstAccessSpecifier;
  std::string TemplateParameterList;
  std::stack<std::pair<AccessSpecifierKind, size_t>> AccessSpecifierStack;
};
} // namespace mergebot::sa

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TYPEDECLNODE_H
