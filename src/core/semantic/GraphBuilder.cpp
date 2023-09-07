//
// Created by whalien on 30/04/23.
//

#include "mergebot/core/semantic/GraphBuilder.h"

#include "mergebot/core/magic_enum_customization.h" // for magic_enum::enum_name
#include "mergebot/core/model/node/IfDefBlockNode.h"
#include "mergebot/core/model/node/NamespaceNode.h"
#include "mergebot/core/model/node/TranslationUnitNode.h"
#include "mergebot/filesystem.h"
#include "mergebot/lsp/client.h"
#include "mergebot/parser/parser.h"
#include "mergebot/parser/utils.h"
#include "mergebot/utils/fileio.h"
#include <magic_enum.hpp>
#include <nlohmann/json.hpp> // for std::vector deserialization
#include <spdlog/spdlog.h>
#include <string_view>

namespace mergebot {
namespace details {
bool IsCppSource(std::string_view path) {
  using namespace std::string_view_literals;
  std::unordered_set<std::string_view> cpp_exts = {
      ".h", ".hpp", ".cc", ".cp", ".C", ".cxx", ".cpp", ".c++"};
  auto pos = path.find_last_of("."sv);
  if (pos == std::string_view::npos) {
    return false;
  }
  std::string_view ext = path.substr(pos);
  return cpp_exts.count(ext);
}

bool IsCppHeader(std::string_view Path) {
  using namespace std::string_view_literals;
  std::unordered_set<std::string_view> cpp_header_exts = {".h", ".hpp"};
  auto pos = Path.find_last_of("."sv);
  if (pos == std::string_view::npos) {
    return false;
  }
  std::string_view ext = Path.substr(pos);
  return cpp_header_exts.count(ext);
}

bool IsCSource(std::string_view Path) {
  std::unordered_set<std::string_view> c_exts = {".h", ".c"};
  // TODO(hwa): enable when support C lang is needed
  return false;
}
} // namespace details

namespace sa {
bool GraphBuilder::build() {
  spdlog::info("building graph for {} side...", magic_enum::enum_name(S));

  spdlog::debug("size of {} Side's sources to be analyzed: {}",
                magic_enum::enum_name(S), SourceList.size());

  bool Success = initLanguageServer();
  if (!Success) {
    spdlog::error("cannot initialize language server");
    return false;
  }

  //  for (std::string const &Path : SourceList) {
  //    processTranslationUnit(Path);
  //  }
  for (size_t i = 0; i < 5; i++) {
    processTranslationUnit(SourceList[i]);
  }
  return true;
}

/// carefully share memory
void GraphBuilder::processTranslationUnit(const std::string &Path) {
  spdlog::info("Side: [{}], processing compilation unit {}...",
               magic_enum::enum_name(S), Path);

  bool IsConflicting = isConflicting(Path);
  std::string FilePath = (fs::path(SourceDir) / Path).string();
  if (!fs::exists(FilePath)) {
    spdlog::warn("Side: [{}], compilation unit {} doesn't exist",
                 magic_enum::enum_name(S), FilePath);
    return;
  }

  if (details::IsCppSource(Path)) {
    processCppTranslationUnit(Path, FilePath, IsConflicting);
  } else {
    spdlog::info("Side: [{}], compilation unit {} is not a C++ source file. We "
                 "skip it currently",
                 magic_enum::enum_name(S), FilePath);
  }
}

void GraphBuilder::processCppTranslationUnit(const std::string &Path,
                                             const std::string &FilePath,
                                             bool IsConflicting) {
  std::string FileSource = util::file_get_content(FilePath);
  /// TODO(hwa): add macro replace here
  /// replace macro to magic string /*MB_MR_BG*/ MACRO /*MB_MR_ED*/

  Client.DidOpen(FilePath, FileSource);
  auto URIOpt = Client.SwitchSourceHeader(FilePath);
  lsp::URIForFile AltUri;
  bool HasAltFile = URIOpt.has_value() && URIOpt.value() != nullptr;
  if (HasAltFile) {
    std::string AltFileScheme = URIOpt.value();
    AltUri = lsp::URIForFile(AltFileScheme);
    std::string AltFilePath = AltUri.path();
    std::string AltFileContent = util::file_get_content(AltFilePath);
    Client.DidOpen(AltUri, AltFileContent);
  }

  ts::Parser PS(ts::cpp::language());
  std::shared_ptr<ts::Tree> Tree = PS.parse(FileSource);
  if (!Tree) {
    spdlog::error("Side: [{}], compilation unit {} cannot be parsed",
                  magic_enum::enum_name(S), FilePath);
    return;
  }
  ts::Node TSRoot = Tree->rootNode();
  assert(TSRoot.type() ==
         mergebot::ts::cpp::symbols::sym_translation_unit.name);

  size_t FrontDeclCnt = 0;
  ts::Node TURoot = TSRoot;
  std::shared_ptr<SemanticNode> TUPtr = parseTranslationUnit(
      TSRoot, IsConflicting, Path, FilePath, FrontDeclCnt, TURoot);
  addVertex(TUPtr);
  parseCompositeNode(TUPtr, IsConflicting, TSRoot, FilePath, FrontDeclCnt);

  // build edges

  // generate context info

  Client.DidClose(FilePath);
  if (HasAltFile) {
    Client.DidClose(AltUri);
  }
}

void GraphBuilder::parseCompositeNode(std::shared_ptr<SemanticNode> &SRoot,
                                      bool IsConflicting,
                                      const ts::Node &TSRoot,
                                      const std::string &FilePath,
                                      const int FirstChildIdx /* = 0 */) {
  static std::unordered_set<std::string> CompositeTypes = {
      "namespace_definition",
  };
  static std::unordered_set<std::string> TerminalTypes = {
      "preproc_ifdef",
      "function_definition", // inline namespace and function definition
      "field_declaration", // function declaration or field declaration in type
      "declaration",       // function or field
      "enumerator",
      "friend_declaration", // function decl, function definition, class
      "alias_declaration",   "using_declaration",
      "typedef_definition",  "comment"};
  static std::unordered_set<std::string> ComplexTypes = {
      "linkage_specification", // declaration or composite type
      "class_specifier",       "struct_specifier",
      "union_specifier",       "enum_specifier",
  };

  for (size_t Idx = FirstChildIdx; Idx < TSRoot.childrenCount();) {
    const ts::Node &Child = TSRoot.children[Idx];
    const std::string ChildType = Child.type();
    if (CompositeTypes.count(ChildType)) {
      std::shared_ptr<SemanticNode> NamespacePtr =
          parseNamespaceNode(Child, IsConflicting, FilePath);
      addVertex(NamespacePtr);
      SRoot->Children.push_back(NamespacePtr);
      parseCompositeNode(NamespacePtr, IsConflicting, Child, FilePath);
    } else if (TerminalTypes.count(ChildType)) {
      if (ts::cpp::symbols::sym_declaration.name == ChildType) {
      }
    } else if (ComplexTypes.count(ChildType)) {

    } else {
      spdlog::error(
          "unexpected node: node type is {}, isNamed is {}, text is {}",
          Child.type(), Child.isNamed(), Child.text());
    }
    ++Idx;
  }
}

std::shared_ptr<TranslationUnitNode> GraphBuilder::parseTranslationUnit(
    const ts::Node &Node, bool IsConflicting, const std::string &Path,
    const std::string &FilePath, size_t &FrontDeclCnt, ts::Node &TURoot) {
  auto CommentPair = ts::getTranslationUnitComment(Node);
  FrontDeclCnt = CommentPair.first;
  std::string Comment = CommentPair.second;

  // 如果是头文件，有传统的header guard，那么TSRoot是一个preproc_ifdef，
  // 并且FrontDeclCnt同时也要进行更新；如果是头文件或实现文件，有#pragma header
  // guard 或无header guard，那么TSRoot是translation_unit; FrontDeclCnt不用更新
  bool IsHeader = false;
  bool TraditionGuard = false;
  std::vector<std::string> HeaderGuard(3);
  if (details::IsCppHeader(FilePath)) {
    IsHeader = true;
    auto [Tradition, Guard] = ts::getHeaderGuard(Node, FrontDeclCnt, TURoot);
    if (Guard.size()) {
      TraditionGuard = Tradition;
      HeaderGuard = std::move(Guard);
    }
  }

  std::vector<std::pair<ts::Point, std::string>> FrontDecls =
      ts::getFrontDecls(TURoot, FrontDeclCnt);

  size_t BeforeFirstChildEOL = 1;
  if (FrontDecls.size()) {
    BeforeFirstChildEOL = TURoot.children[FrontDeclCnt].startPoint().row -
                          FrontDecls.back().first.row - 1;
  }

  return std::make_shared<TranslationUnitNode>(
      NodeCount++, IsConflicting, NodeType::TRANSLATION_UNIT, Path,
      Meta.ProjectPath, FilePath, std::move(Comment), std::nullopt,
      std::string(Path), Path, IsHeader, TraditionGuard, std::move(HeaderGuard),
      std::move(FrontDecls), BeforeFirstChildEOL);
}

std::shared_ptr<NamespaceNode>
GraphBuilder::parseNamespaceNode(const ts::Node &Node, bool IsConflicting,
                                 const std::string &Path) {
  // cascade, normal, anonymous, inline
  std::string DisplayName;
  std::string OriginalSignature;
  int NOffset = -1;
  RE2 pattern(R"(((inline\s+)?namespace\s*([^\s{]*))\s*\{)");
  re2::StringPiece input(Node.text().data(), Node.text().size());
  if (RE2::PartialMatch(input, pattern, &OriginalSignature, nullptr,
                        &DisplayName)) {
    if (!DisplayName.empty()) {
      NOffset = input.find(DisplayName) - 1;
    }
  }

  std::string USR;           // symbol details获取USR
  std::string QualifiedName; // symbol details 获取QualifiedName
  int row = Node.startPoint().row;
  int col = Node.startPoint().column;
  if (NOffset != -1) {
    col += NOffset;
  }
  std::optional<lsp::SymbolDetails> detailsOpt =
      getSymbolDetails(Path, lsp::Position{row, col});
  if (detailsOpt.has_value()) {
    const lsp::SymbolDetails &details = detailsOpt.value();
    USR = details.usr;
    QualifiedName = details.containerName.value_or("");
    // if clangd gives us a name, we use the clangd name
    if (details.name.size()) {
      DisplayName = details.name;
    }
  }

  std::string Comment = getNodeComment(Node);
  size_t BeforeFirstChildEOL = ts::beforeFirstChildEOLs(Node);

  std::string NSComment;
  if (Node.nextSibling()->type() == ts::cpp::symbols::sym_comment.name) {
    NSComment = Node.nextSibling()->text();
  }

  return std::make_shared<NamespaceNode>(
      NodeCount++, IsConflicting, NodeType::NAMESPACE, DisplayName,
      QualifiedName, OriginalSignature, std::move(Comment), Node.startPoint(),
      std::move(USR), BeforeFirstChildEOL, std::move(NSComment));
}

bool GraphBuilder::isConflicting(std::string_view Path) const {
  return std::find(ConflictPaths.begin(), ConflictPaths.end(), Path) !=
         ConflictPaths.end();
}

std::optional<lsp::SymbolDetails>
GraphBuilder::getSymbolDetails(const lsp::URIForFile &URI,
                               const lsp::Position Pos) {
  auto returned = Client.SymbolInfo(URI, Pos);
  if (!returned.has_value()) {
    return std::nullopt;
  }
  std::vector<lsp::SymbolDetails> details = returned.value();
  if (!details.size()) {
    return std::nullopt;
  }
  return details[0];
}

bool GraphBuilder::initLanguageServer() {
  auto Communicator = lsp::PipeCommunicator::create("./clangd", "clangd");
  if (!Communicator) {
    spdlog::error("cannot create pipe to communicate with child process");
    return false;
  }
  std::unique_ptr<lsp::JSONRpcEndpoint> RpcEndpoint =
      std::make_unique<lsp::JSONRpcEndpoint>(std::move(Communicator));
  std::unique_ptr<lsp::LspEndpoint> LspEndpoint =
      std::make_unique<lsp::LspEndpoint>(std::move(RpcEndpoint), 3);

  Client = lsp::LspClient(std::move(LspEndpoint));

  std::string WorkspaceRoot =
      (fs::path(Meta.MSCacheDir) / magic_enum::enum_name(S)).string();
  auto InitializedResult = Client.Initialize(WorkspaceRoot);
  if (InitializedResult.has_value()) {
    spdlog::debug("server info: {}",
                  InitializedResult.value()["serverInfo"]["version"]);
  }
  return true;
}

GraphBuilder::vertex_descriptor
GraphBuilder::addVertex(std::shared_ptr<SemanticNode> Node) {
  // TODO(hwa): will there be a memory leak?
  return boost::add_vertex(Node, G);
}

GraphBuilder::~GraphBuilder() {
  Client.Shutdown();
  Client.Exit();
}
} // namespace sa
} // namespace mergebot