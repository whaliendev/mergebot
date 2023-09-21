//
// Created by whalien on 10/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_FUNCDEFNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_FUNCDEFNODE_H

#include "mergebot/core/model/node/TerminalNode.h"
#include <algorithm>
namespace mergebot {
namespace sa {
/// trailing return type or plain function
class FuncDefNode : public TerminalNode {
public:
  FuncDefNode(int NodeId, bool NeedToMerge, NodeKind Kind,
              const std::string &DisplayName, const std::string &QualifiedName,
              const std::string &OriginalSignature, std::string &&Comment,
              const std::optional<ts::Point> &Point, std::string &&USR,
              std::string &&Body, size_t ParentSignatureHash,
              size_t FollowingEOL, std::string &&TemplateParameterList,
              std::string &&Attrs, std::string &&BeforeFuncName,
              std::vector<std::string> &&ParameterList,
              std::string &&AfterParameterList, bool IsSynthetic = false)
      : TerminalNode(NodeId, NeedToMerge, Kind, DisplayName, QualifiedName,
                     OriginalSignature, std::move(Comment), Point,
                     std::move(USR), std::move(Body), ParentSignatureHash,
                     FollowingEOL, IsSynthetic),
        TemplateParameterList(std::move(TemplateParameterList)),
        Attrs(std::move(Attrs)), BeforeFuncName(std::move(BeforeFuncName)),
        ParameterList(std::move(ParameterList)),
        AfterParameterList(std::move(AfterParameterList)) {}

  static bool classof(const SemanticNode *N) {
    return N->getKind() == NodeKind::FUNC_DEF;
  }

  size_t hashSignature() const override {
    size_t H = 1;
    mergebot::hash_combine(H, getKind());
    if (!USR.empty()) {
      mergebot::hash_combine(H, USR);
      return H;
    }

    // visibility change is a refactoring, we intentionally don't count it
    mergebot::hash_combine(H, QualifiedName);
    //    mergebot::hash_combine(H, getKind());
    //    mergebot::hash_combine(H, this->QualifiedName);
    //    mergebot::hash_combine(H, VectorHash<std::string>{}(ParameterTypes));
    //    std::string TmpStr = AfterParameterList;
    //    TmpStr.erase(std::remove_if(TmpStr.begin(), TmpStr.end(), ::isspace),
    //                 TmpStr.end());
    //    mergebot::hash_combine(H, TmpStr);
    return H;
  }

  std::string TemplateParameterList;
  std::string Attrs;
  std::string BeforeFuncName; // return value, virtual, storage specifier, etc.
  std::vector<std::string> ParameterList;
  std::string AfterParameterList;

  // extract from USR for USE edge
  std::vector<std::string> References;
  std::vector<std::string> ParameterTypes;
};

} // namespace sa
} // namespace mergebot

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_FUNCDEFNODE_H
