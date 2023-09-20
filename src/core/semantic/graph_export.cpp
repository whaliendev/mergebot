//
// Created by whalien on 19/09/23.
//
#include "mergebot/core/semantic/graph_export.h"

namespace mergebot::sa {
bool ExportGraphToDot(const GraphBuilder::SemanticGraph &g,
                      const std::string &filename, bool showSynthetic) {
  std::ofstream destStream;
  destStream.open(filename, std::ios::out | std::ios::trunc);
  if (!destStream.is_open()) {
    return false;
  }
  boost::write_graphviz(destStream, g, SemanticNodeWriter(g, showSynthetic),
                        SemanticEdgeWriter(g, showSynthetic));
  destStream.close();
  return true;
}

bool ExportGraphToStdOut(const GraphBuilder::SemanticGraph &g,
                         bool showSynthetic) {
  boost::write_graphviz(std::cout, g, SemanticNodeWriter(g, showSynthetic),
                        SemanticEdgeWriter(g, showSynthetic));
  return true;
}

} // namespace mergebot::sa
