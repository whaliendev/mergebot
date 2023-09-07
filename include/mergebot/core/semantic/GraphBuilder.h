//
// Created by whalien on 30/04/23.
//

#ifndef MB_GRAPH_BUILDER_H
#define MB_GRAPH_BUILDER_H

#include "mergebot/core/handler/SAHandler.h"
#include "mergebot/core/magic_enum_customization.h"
#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/core/model/SemanticEdge.h"
#include "mergebot/core/model/SemanticNode.h"
#include "mergebot/core/model/Side.h"
#include "mergebot/filesystem.h"
#include "mergebot/lsp/client.h"
#include "mergebot/parser/tree.h"
#include <boost/graph/graph_selectors.hpp>
#include <magic_enum.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/graph/adjacency_list.hpp>
namespace mergebot {
namespace sa {
class NamespaceNode;
class TranslationUnitNode;

class GraphBuilder {
public:
  typedef boost::adjacency_list<
      boost::listS,          // Store out-edges of each vertex in std::list
      boost::vecS,           // Store vertex set in std::vector
      boost::bidirectionalS, // The graph is bidirectional
      std::shared_ptr<SemanticNode>, // Vertex properties
      SemanticEdge                   // Edge properties
      >
      SemanticGraph;

  GraphBuilder(Side S, ProjectMeta const &Meta,
               std::vector<std::string> const &ConflictPaths, // relative paths
               std::vector<std::string> const &SourceList,    // relative paths
               std::unordered_map<std::string, std::vector<std::string>> const
                   &DirectIncluded)
      : S(S), Meta(Meta),
        ConflictPaths(ConflictPaths.begin(), ConflictPaths.end()),
        SourceList(SourceList), DirectIncluded(DirectIncluded) {
    SourceDir = (fs::path(Meta.MSCacheDir) / magic_enum::enum_name(S)).string();
  }

  /// shutdown language server
  ~GraphBuilder();

  bool build();
  SemanticGraph graph() const { return G; }

private:
  using edge_iterator = SemanticGraph::edge_iterator;
  using vertex_iterator = SemanticGraph::vertex_iterator;
  using vertex_descriptor = SemanticGraph::vertex_descriptor;
  using edge_descriptor = SemanticGraph::edge_descriptor;

  void processTranslationUnit(const std::string &Path);

  void processCppTranslationUnit(const std::string &Path,
                                 const std::string &FilePath,
                                 bool IsConflicting);

  void parseCompositeNode(std::shared_ptr<SemanticNode> &SRoot,
                          bool IsConflicting, const ts::Node &Root,
                          const std::string &Path, const int FirstChildIdx = 0);
  std::shared_ptr<TranslationUnitNode>
  parseTranslationUnit(const ts::Node &Node, bool IsConflicting,
                       const std::string &Path, const std::string &FilePath,
                       size_t &FrontDeclCnt, ts::Node &TURoot);
  std::shared_ptr<NamespaceNode> parseNamespaceNode(const ts::Node &Node,
                                                    bool IsConflicting,
                                                    const std::string &Path);

  bool initLanguageServer();
  std::optional<lsp::SymbolDetails> getSymbolDetails(const lsp::URIForFile &URI,
                                                     const lsp::Position Pos);

  bool isConflicting(std::string_view Path) const;

  vertex_descriptor addVertex(std::shared_ptr<SemanticNode> Node);

  lsp::LspClient Client;

  /// build for which side
  Side S;
  /// for retrieving cached merge scenario sources
  ProjectMeta Meta;
  /// specify which files need to be merged
  std::unordered_set<std::string> ConflictPaths;
  /// delta sources: add, rename, modify, delete, etc.
  std::vector<std::string> SourceList;
  /// mapping of source's direct included files for each source in `SourceList`
  std::unordered_map<std::string, std::vector<std::string>> DirectIncluded;

  SemanticGraph G;

  std::string SourceDir;

  int NodeCount = 0;
  int EdgeCount = 0;

  /**
   * a series of temp containers to store relationships among nodes and symbols
   */
  /// Inclusion analysis of conflicts caused by long-standing unmerged branches
  /// can be difficult
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
};

} // namespace sa
} // namespace mergebot

#endif // MB_GRAPH_BUILDER_H
