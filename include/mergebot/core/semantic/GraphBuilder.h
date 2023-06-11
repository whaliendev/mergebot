//
// Created by whalien on 30/04/23.
//

#ifndef MB_GRAPHBUILDER_H
#define MB_GRAPHBUILDER_H

#include "mergebot/core/handler/SAHandler.h"
#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/core/model/SemanticEdge.h"
#include "mergebot/core/model/SemanticNode.h"
#include "mergebot/core/model/Side.h"
#include <boost/graph/graph_selectors.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/graph/adjacency_list.hpp>
namespace mergebot {
namespace sa {

class GraphBuilder {
public:
  typedef boost::adjacency_list<boost::vecS, boost::listS,
                                boost::bidirectionalS,
                                std::shared_ptr<SemanticNode>, SemanticEdge>
      Graph;

  GraphBuilder(Side S, ProjectMeta const &Meta,
               std::vector<std::string> const &ConflictPaths,
               std::vector<std::string> const &SourceList,
               std::unordered_map<std::string, std::vector<std::string>> const
                   &DirectIncluded)
      : S(S), Meta(Meta), ConflictPaths(ConflictPaths), SourceList(SourceList),
        DirectIncluded(DirectIncluded) {}

  void build();
  Graph graph() const { return G; }

private:
  void processTranslationUnit(std::string_view Path);
  /// build for which side
  Side S;
  /// for retrieving cached merge scenario sources
  ProjectMeta Meta;
  /// specify which files need to be merged
  std::vector<std::string> ConflictPaths;
  /// delta sources: add, rename, modify, delete, etc.
  std::vector<std::string> SourceList;
  /// mapping of source's direct included files for each source in `SourceList`
  std::unordered_map<std::string, std::vector<std::string>> DirectIncluded;

  Graph G;

  int NodeCount = 0;
  int EdgeCount = 0;

  /**
   * a series of temp containers to store relationships among nodes and symbols
   */
  std::unordered_map<std::shared_ptr<SemanticNode>, std::vector<std::string>>
      IncludeEdges;

  std::unordered_map<std::shared_ptr<SemanticNode>, std::vector<std::string>>
      InheritEdges;

  std::unordered_map<std::shared_ptr<SemanticNode>, std::vector<std::string>>
      DeclEdges;

  std::unordered_map<std::shared_ptr<SemanticNode>, std::vector<std::string>>
      ReadFieldEdges;

  std::unordered_map<std::shared_ptr<SemanticNode>, std::vector<std::string>>
      WriteFieldEdges;

  std::unordered_map<std::shared_ptr<SemanticNode>, std::vector<std::string>>
      FunctionCallEdges;

  using edge_iterator = Graph::edge_iterator;
  using vertex_iterator = Graph::vertex_iterator;
  using vertex_descripor = Graph::vertex_descriptor;
  using edge_descriptor = Graph::edge_descriptor;
};

} // namespace sa
} // namespace mergebot

#endif // MB_GRAPHBUILDER_H
