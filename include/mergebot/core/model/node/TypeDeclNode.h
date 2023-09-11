//
// Created by whalien on 03/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TYPEDECLNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TYPEDECLNODE_H

#include "mergebot/core/model/node/CompositeNode.h"
#include <queue>

namespace mergebot::sa {
// class, struct, union
class TypeDeclNode : public CompositeNode {
public:
  enum class TypeDeclKind : uint8_t {
    Class,
    Struct,
    Union,
  };

  TypeDeclNode(int NodeId, bool NeedToMerge, NodeKind NKind,
               const std::string &DisplayName, const std::string &QualifiedName,
               const std::string &OriginalSignature, std::string &&Comment,
               const std::optional<ts::Point> &Point, std::string &&USR,
               size_t BeforeFirstChildEOL, TypeDeclKind Kind,
               std::string &&Attrs, bool IsFinal, std::string &&BaseClause,
               AccessSpecifierKind FirstAccessSpecifier,
               std::string &&TemplateParameterList)
      : CompositeNode(NodeId, NeedToMerge, NKind, DisplayName, QualifiedName,
                      OriginalSignature, std::move(Comment), Point,
                      std::move(USR), BeforeFirstChildEOL),
        Kind(Kind), Attrs(std::move(Attrs)), IsFinal(IsFinal),
        BaseClause(std::move(BaseClause)),
        FirstAccessSpecifier(FirstAccessSpecifier),
        TemplateParameterList(std::move(TemplateParameterList)) {
    int idx = FirstAccessSpecifier == AccessSpecifierKind::Default ? -1 : 0;
    this->AccessSpecifierQue.push(std::make_pair(FirstAccessSpecifier, idx));
  }

  static bool classof(const SemanticNode *N) {
    return N->getKind() == NodeKind::TYPE;
  }

  void setMemberAccessSpecifier() {
    assert(!this->AccessSpecifierQue.empty() && "AccessSpecifierQue is empty");
    int ChildIdx = 0;
    while (!AccessSpecifierQue.empty()) {
      auto [AccessSpecifier, Start] = AccessSpecifierQue.front();
      AccessSpecifierQue.pop();

      int End = -1;
      if (!AccessSpecifierQue.empty()) {
        auto SpecifierPair = AccessSpecifierQue.front();
        End = SpecifierPair.second;
      }

      int LoopEnd =
          End == -1 ? this->Children.size() : ChildIdx + End - Start - 1;

      for (int idx = ChildIdx; idx < LoopEnd; ChildIdx = ++idx) {
        this->Children[idx]->AccessSpecifier = AccessSpecifier;
      }
    }
  }

public:
  TypeDeclKind Kind;
  std::string Attrs;
  bool IsFinal;
  std::string BaseClause;
  AccessSpecifierKind FirstAccessSpecifier;
  std::string TemplateParameterList;
  std::queue<std::pair<AccessSpecifierKind, int>> AccessSpecifierQue;
};
} // namespace mergebot::sa

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TYPEDECLNODE_H
