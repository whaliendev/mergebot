//
// Created by whalien on 03/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TRANSLATIONUNITNODE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TRANSLATIONUNITNODE_H

#include "mergebot/core/model/node/CompositeNode.h"
#include <set>

namespace mergebot {
namespace sa {
class TranslationUnitNode : public CompositeNode {
public:
  TranslationUnitNode(
      int NodeId, bool NeedToMerge, NodeKind Kind,
      const std::string &DisplayName, const std::string &QualifiedName,
      const std::string &OriginalSignature, std::string &&Comment,
      const std::optional<ts::Point> &Point, std::string &&USR, bool IsHeader,
      bool TraditionGuard, std::vector<std::string> &&HeaderGuard,
      std::vector<std::pair<ts::Point, std::string>> &&FrontDecls,
      size_t BeforeFirstChildEOL, bool IsSynthetic = false)
      : CompositeNode(NodeId, NeedToMerge, Kind, DisplayName, QualifiedName,
                      OriginalSignature, std::move(Comment), Point,
                      std::move(USR), BeforeFirstChildEOL, IsSynthetic),
        IsHeader(IsHeader), TraditionGuard(TraditionGuard),
        HeaderGuard(std::move(HeaderGuard)),
        FrontDecls(std::move(FrontDecls)){};

  static bool classof(const SemanticNode *N) {
    return N->getKind() == NodeKind::TRANSLATION_UNIT;
  }

  size_t hashSignature() const override {
    size_t H = 1;
    mergebot::hash_combine(H, getKind());
    mergebot::hash_combine(H, this->QualifiedName);
    return H;
  }

  bool IsHeader = false;
  bool TraditionGuard = true;
  /// @code
  /// #ifndef xxx
  /// #define xxx
  /// #endif
  /// @endcode
  /// or
  /// @code
  /// #pragma once
  /// @endcode
  std::vector<std::string> HeaderGuard;
  /// @code
  /// #include <cstdio>
  /// #include <map>
  /// #ifdef __GNU__
  /// babala...
  /// #endif
  /// // this is some comment for this or for that
  /// extern A* doSomething();
  /// constexpr int kN = 3;
  /// extern int kM;
  /// using fs = std::filesystem;
  /// typedef std::map<int, int> IntMap;
  /// @endcode
  std::vector<std::pair<ts::Point, std::string>> FrontDecls;
};
} // namespace sa
} // namespace mergebot

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_NODE_TRANSLATIONUNITNODE_H
