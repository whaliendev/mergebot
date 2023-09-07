//
// Created by whalien on 08/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_MAPPING_NODECONTEXT_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_MAPPING_NODECONTEXT_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace mergebot {
namespace sa {
class SemanticEdge;

class NodeContext {
public:
  NodeContext() = default;
  // one-hop similarity
  //  std::unordered_set<SemanticEdge> InEdges;
  //  std::unordered_set<SemanticEdge> OutEdges;
  //  std::unordered_map<int, int> InVec;
  //  std::unordered_map<int, int> OutVec;
  //
  //  // references and declarations, context similarity
  //  std::vector<std::string> Refs;
  //  std::vector<std::string> Decls;
};
} // namespace sa
} // namespace mergebot

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_MAPPING_NODECONTEXT_H
