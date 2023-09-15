//
// Created by whalien on 03/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TYPEDECLNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TYPEDECLNODE_H

#include "AccessSpecifierNode.h"
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
               size_t BeforeFirstChildEOL, TypeDeclKind Type,
               std::string &&Attrs, bool IsFinal, std::string &&BaseClause,
               std::string &&TemplateParameterList, bool IsSynthetic = false)
      : CompositeNode(NodeId, NeedToMerge, NKind, DisplayName, QualifiedName,
                      OriginalSignature, std::move(Comment), Point,
                      std::move(USR), BeforeFirstChildEOL, IsSynthetic),
        Type(Type), Attrs(std::move(Attrs)), IsFinal(IsFinal),
        BaseClause(std::move(BaseClause)),
        TemplateParameterList(std::move(TemplateParameterList)) {}

  static bool classof(const SemanticNode *N) {
    return N->getKind() == NodeKind::TYPE;
  }

  size_t hashSignature() const override {
    size_t H = 1;
    mergebot::hash_combine(H, getKind());
    mergebot::hash_combine(H, Kind);
    mergebot::hash_combine(H, DisplayName);
    mergebot::hash_combine(H, IsFinal);
    mergebot::hash_combine(H, BaseClause);
    return H;
  }

  void setMemberAccessSpecifier() {
    if (this->Children.empty()) {
      return;
    }
    AccessSpecifierKind AccessKind = AccessSpecifierKind::Default;
    std::shared_ptr<SemanticNode> FirstChild = this->Children.front();
    if (auto *AccessSpecifierPtr =
            llvm::dyn_cast<AccessSpecifierNode>(FirstChild.get())) {
      AccessKind = AccessSpecifierPtr->AccessKind;
    }
    for (size_t idx = 1; idx < this->Children.size(); ++idx) {
      if (!llvm::isa<AccessSpecifierNode>(this->Children[idx].get())) {
        this->Children[idx]->AccessSpecifier = AccessKind;
      } else {
        AccessKind = llvm::cast<AccessSpecifierNode>(this->Children[idx].get())
                         ->AccessKind;
      }
    }
  }

public:
  TypeDeclKind Type;
  std::string Attrs;
  bool IsFinal;
  std::string BaseClause;
  std::string TemplateParameterList;
};
} // namespace mergebot::sa

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TYPEDECLNODE_H
