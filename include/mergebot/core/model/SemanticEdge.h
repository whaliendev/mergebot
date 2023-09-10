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
  int i = 4;

private:
  int ID;
  EdgeKind Type;
  int Weight;
  std::shared_ptr<SemanticNode> Source;
  std::shared_ptr<SemanticNode> Target;
  /// whether the target node is defined inside the graph or not
  bool IsInternal;
};

} // namespace sa
} // namespace mergebot

#endif // MB_SRC_CORE_MODEL_SEMANTICEDGE_H
