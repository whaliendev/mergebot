//
// Created by whalien on 30/04/23.
//

#ifndef MB_GRAPHEXPORTER_H
#define MB_GRAPHEXPORTER_H
#include "mergebot/core/semantic/GraphBuilder.h"
#include "mergebot/utils/fmt_escape.h"
#include <boost/graph/graphviz.hpp>
#include <fmt/core.h>
#include <fstream>
namespace mergebot::sa {
constexpr const char *getNodeKindShape(NodeKind Kind) {
  switch (Kind) {
  case NodeKind::TRANSLATION_UNIT:
    return "doublecircle";
  case NodeKind::LINKAGE_SPEC_LIST:
    return "diamond";
  case NodeKind::NAMESPACE:
    return "box3d";
  case NodeKind::TYPE:
  case NodeKind::ENUM:
    return "ellipse";
  case NodeKind::FIELD_DECLARATION:
    return "polygon";
  case NodeKind::FUNC_DEF:
  case NodeKind::FUNC_OPERATOR_CAST:
  case NodeKind::FUNC_SPECIAL_MEMBER:
    return "oval";
  case NodeKind::ORPHAN_COMMENT:
    return "note";
  case NodeKind::TEXTUAL:
    return "rect";
  case NodeKind::ACCESS_SPECIFIER:
    return "plain";
  default:
    return "plaintext";
  }
}

struct SemanticNodeWriter {
  explicit SemanticNodeWriter(const GraphBuilder::SemanticGraph &g) : g(g) {}

  template <class VertexDesc>
  void operator()(std::ostream &out, const VertexDesc &v) const {
    const std::shared_ptr<SemanticNode> &node = g[v];
    out << fmt::format("[label={}, type={}, shape={}]",
                       util::escaped(node->QualifiedName + node->DisplayName),
                       magic_enum::enum_name(node->getKind()),
                       getNodeKindShape(node->getKind()));
  }

  const GraphBuilder::SemanticGraph &g;
};

struct SemanticEdgeWriter {
  explicit SemanticEdgeWriter(const GraphBuilder::SemanticGraph &g,
                              bool showSynthetic = true)
      : g(g), showSynthetic(showSynthetic) {}

  template <class EdgeDesc>
  void operator()(std::ostream &out, const EdgeDesc &e) const {
    const SemanticEdge &edge = g[e];
    if (!showSynthetic && !edge.IsPhysical) {
      out << fmt::format("[style=invis]");
      return;
    }
    out << fmt::format("[label={}, weight={}]",
                       magic_enum::enum_name(edge.Kind), edge.Weight);
  }

  const GraphBuilder::SemanticGraph &g;
  bool showSynthetic;
};

bool ExportGraphToDot(const GraphBuilder::SemanticGraph &g,
                      const std::string &filename, bool showSynthetic = true) {
  std::ofstream destStream;
  destStream.open(filename, std::ios::out | std::ios::trunc);
  if (!destStream.is_open()) {
    return false;
  }
  boost::write_graphviz(destStream, g, SemanticNodeWriter(g),
                        SemanticEdgeWriter(g, showSynthetic));
  destStream.close();
  return true;
}

bool ExportGraphToStdOut(const GraphBuilder::SemanticGraph &g,
                         bool showSynthetic = true) {
  boost::write_graphviz(std::cout, g, SemanticNodeWriter(g),
                        SemanticEdgeWriter(g, showSynthetic));
  return true;
}

} // namespace mergebot::sa

#endif // MB_GRAPHEXPORTER_H
