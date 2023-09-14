//
// Created by whalien on 08/05/23.
//

#ifndef MB_SRC_CORE_MODEL_SEMANTICEDGE_H
#define MB_SRC_CORE_MODEL_SEMANTICEDGE_H

#include "mergebot/core/model/enum/EdgeKind.h"
#include <memory>
namespace mergebot {
namespace sa {
class SemanticNode;
class SemanticEdge {
public:
  SemanticEdge(int ID, EdgeKind Kind, int Weight, bool IsSynthetic)
      : ID(ID), Kind(Kind), Weight(Weight), IsSynthetic(IsSynthetic) {}

  SemanticEdge(int ID, EdgeKind Kind)
      : ID(ID), Kind(Kind), Weight(1), IsSynthetic(false) {}

  SemanticEdge()
      : ID(-1), Kind(EdgeKind::ILLEGAL), Weight(-1), IsSynthetic(true) {}

  int ID;
  EdgeKind Kind;
  int Weight; // how many times it appears
  bool IsSynthetic;
};

} // namespace sa
} // namespace mergebot

#endif // MB_SRC_CORE_MODEL_SEMANTICEDGE_H
