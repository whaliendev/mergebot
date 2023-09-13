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
#include "mergebot/core/model/node/TypeDeclNode.h"
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
class TextualNode;
class FieldDeclarationNode;
class LinkageSpecNode;
class EnumNode;
class FuncDefNode;
class FuncSpecialMemberNode;
class FuncOperatorCastNode;

class GraphBuilder {
public:
  typedef boost::adjacency_list<
      boost::listS,     // Store out-edges of each vertex in std::list
      boost::vecS,      // Store vertex set in std::vector
      boost::directedS, // The graph is directed
      std::shared_ptr<SemanticNode>, // Vertex properties
      SemanticEdge                   // Edge properties
      >
      SemanticGraph;

  GraphBuilder(Side S, ProjectMeta const &Meta,
               std::vector<std::string> const &ConflictPaths, // relative paths
               std::vector<std::string> const &SourceList,    // relative paths
               std::unordered_map<std::string, std::vector<std::string>> const
                   &DirectIncluded,
               bool OnlyHeaderSourceMapping = true)
      : S(S), Meta(Meta),
        ConflictPaths(ConflictPaths.begin(), ConflictPaths.end()),
        SourceList(SourceList), DirectIncluded(DirectIncluded),
        OnlyHeaderSourceMapping(OnlyHeaderSourceMapping) {
    SourceDir = (fs::path(Meta.MSCacheDir) / magic_enum::enum_name(S)).string();

    if (!OnlyHeaderSourceMapping) {
      std::unordered_set<std::string> IncludedFiles;
      std::for_each(
          this->DirectIncluded.begin(), this->DirectIncluded.end(),
          [&](std::pair<const std::string, std::vector<std::string>> &Pair) {
            IncludedFiles.insert(Pair.second.begin(), Pair.second.end());
          });

      this->SourceList.insert(this->SourceList.end(), IncludedFiles.begin(),
                              IncludedFiles.end());
    }
  }

  /// shutdown language server
  ~GraphBuilder();

  bool build();
  const SemanticGraph &graph() const { return G; }

  size_t numEdges() const { return boost::num_edges(G); }
  size_t numVertices() const { return boost::num_vertices(G); }

  static std::unordered_set<std::string> CompositeTypes;
  static std::unordered_set<std::string> TerminalTypes;
  static std::unordered_set<std::string> ComplexTypes;

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
                          const vertex_descriptor &V, bool IsConflicting,
                          const ts::Node &Root, const std::string &Path,
                          const int FirstChildIdx = 0);
  std::shared_ptr<TranslationUnitNode>
  parseTranslationUnit(const ts::Node &Node, bool IsConflicting,
                       const std::string &Path, const std::string &FilePath,
                       size_t &FrontDeclCnt, ts::Node &TURoot);
  std::shared_ptr<NamespaceNode> parseNamespaceNode(const ts::Node &Node,
                                                    bool IsConflicting,
                                                    const std::string &Path);
  std::shared_ptr<TextualNode> parseTextualNode(const ts::Node &Node,
                                                bool IsConflicting,
                                                size_t ParentSignatureHash);
  std::shared_ptr<FieldDeclarationNode>
  parseFieldDeclarationNode(const ts::Node &Node, bool IsConflicting,
                            size_t ParentSignatureHash,
                            const std::string &FilePath);
  std::shared_ptr<LinkageSpecNode>
  parseLinkageSpecNode(const ts::Node &Node, bool IsConflicting,
                       size_t ParentSignatureHash);
  std::shared_ptr<EnumNode> parseEnumNode(const ts::Node &Node,
                                          bool IsConflicting,
                                          const std::string &FilePath);
  std::pair<std::shared_ptr<TypeDeclNode>, TypeDeclNode::TypeDeclKind>
  parseTypeDeclNode(const ts::Node &Node, const ts::Node &RealNode,
                    bool IsConflicting, const std::string &Path);

  std::shared_ptr<FuncDefNode> parseFuncDefNode(const ts::Node &Node,
                                                const ts::Node &RealNode,
                                                bool IsConflicting,
                                                const std::string &FilePath);

  std::shared_ptr<FuncOperatorCastNode>
  parseFuncOperatorCastNode(const ts::Node &Node, bool IsConflicting,
                            const std::string &FilePath);

  std::shared_ptr<FuncSpecialMemberNode>
  parseFuncSpecialMemberNode(const ts::Node &Node, bool IsConflicting,
                             const std::string &FilePath);

  bool initLanguageServer();
  std::optional<lsp::SymbolDetails> getSymbolDetails(const lsp::URIForFile &URI,
                                                     const lsp::Position Pos);

  std::vector<std::string> getReferences(const lsp::URIForFile &URI,
                                         const lsp::Position Pos);

  bool isConflicting(std::string_view Path) const;

  vertex_descriptor addVertex(std::shared_ptr<SemanticNode> Node);
  std::pair<edge_descriptor, bool> addEdge(vertex_descriptor Source,
                                           vertex_descriptor Target,
                                           SemanticEdge Edge);

  std::pair<vertex_descriptor, bool>
  insertToGraphAndParent(std::shared_ptr<SemanticNode> &ParentPtr,
                         const vertex_descriptor &ParentDesc,
                         const std::shared_ptr<SemanticNode> &CurPtr);

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

  bool OnlyHeaderSourceMapping;

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
      IncludeEdges; // 通过计算相对路径

  std::unordered_map<std::shared_ptr<SemanticNode>, std::vector<std::string>>
      InheritEdges; // 通过记录继承关系 // identifier

  std::unordered_map<std::shared_ptr<SemanticNode>, std::vector<std::string>>
      implementEdges;

  std::unordered_map<std::shared_ptr<SemanticNode>, std::vector<std::string>>
      ReadFieldEdges; // function里着重注意

  std::unordered_map<std::shared_ptr<SemanticNode>, std::vector<std::string>>
      WriteFieldEdges; // function里着重注意

  std::unordered_map<std::shared_ptr<SemanticNode>, std::vector<std::string>>
      FunctionCallEdges;
};

} // namespace sa
} // namespace mergebot

#endif // MB_GRAPH_BUILDER_H
