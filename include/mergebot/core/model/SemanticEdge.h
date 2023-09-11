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
  SemanticEdge(int ID, EdgeKind Kind, int Weight, bool IsPhysical)
      : ID(ID), Kind(Kind), Weight(Weight), IsPhysical(IsPhysical) {}

  SemanticEdge(int ID, EdgeKind Kind)
      : ID(ID), Kind(Kind), Weight(1), IsPhysical(true) {}

  int ID;
  EdgeKind Kind;
  int Weight;      // how many times it appears
  bool IsPhysical; // physical edge or logical edge
};

} // namespace sa
} // namespace mergebot

#endif // MB_SRC_CORE_MODEL_SEMANTICEDGE_H
