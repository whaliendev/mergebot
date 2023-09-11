//
// Created by whalien on 30/04/23.
//

#include "mergebot/core/semantic/GraphBuilder.h"

#include "mergebot/core/magic_enum_customization.h" // for magic_enum::enum_name
#include "mergebot/core/model/node/EnumNode.h"
#include "mergebot/core/model/node/FieldDeclarationNode.h"
#include "mergebot/core/model/node/FuncDefNode.h"
#include "mergebot/core/model/node/FuncOperatorCastNode.h"
#include "mergebot/core/model/node/FuncSpecialMemberNode.h"
#include "mergebot/core/model/node/LinkageSpecNode.h"
#include "mergebot/core/model/node/NamespaceNode.h"
#include "mergebot/core/model/node/OrphanCommentNode.h"
#include "mergebot/core/model/node/TextualNode.h"
#include "mergebot/core/model/node/TranslationUnitNode.h"
#include "mergebot/core/model/node/TypeDeclNode.h"
#include "mergebot/filesystem.h"
#include "mergebot/lsp/client.h"
#include "mergebot/parser/parser.h"
#include "mergebot/parser/point.h" // for template instantiation for ts::Point
#include "mergebot/parser/utils.h"
#include "mergebot/utils/fileio.h"
#include <magic_enum.hpp>
#include <nlohmann/json.hpp> // for std::vector deserialization
#include <spdlog/spdlog.h>
#include <string_view>

namespace mergebot {
namespace details {

sa::AccessSpecifierKind
getAccessSpecifierKind(std::string_view AccessSpecifier) {
  if (AccessSpecifier.find("public") != std::string_view::npos) {
    return sa::AccessSpecifierKind::Public;
  } else if (AccessSpecifier.find("protected") != std::string_view::npos) {
    return sa::AccessSpecifierKind::Protected;
  } else if (AccessSpecifier.find("private") != std::string_view::npos) {
    return sa::AccessSpecifierKind::Private;
  } else {
    spdlog::error("unexpected access specifier: {}", AccessSpecifier);
    return sa::AccessSpecifierKind::None;
  }
}

bool IsClassSpecifier(std::string_view child_type) {
  namespace symbols = ts::cpp::symbols;
  return child_type == symbols::sym_class_specifier.name ||
         child_type == symbols::sym_struct_specifier.name ||
         child_type == symbols::sym_union_specifier.name;
}

bool IsTextualNode(std::string_view child_type) {
  namespace symbols = ts::cpp::symbols;
  return child_type == symbols::sym_preproc_ifdef.name ||
         child_type == symbols::sym_using_declaration.name ||
         child_type == symbols::sym_type_definition.name ||
         child_type == symbols::sym_alias_declaration.name ||
         child_type == symbols::sym_enumerator.name ||
         child_type == symbols::sym_declaration.name ||
         child_type == symbols::sym_static_assert_declaration.name ||
         child_type == symbols::sym_friend_declaration.name;
}

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
std::unordered_set<std::string> GraphBuilder::CompositeTypes = {
    "namespace_definition",
};

std::unordered_set<std::string> GraphBuilder::TerminalTypes = {
    "access_specifier",
    "preproc_ifdef",             // textual
    "using_declaration",         // textual
    "comment",                   // textual
    "typedef_definition",        // textual
    "alias_declaration",         // textual
    "enumerator",                // textual
    "declaration",               // textual
    "static_assert_declaration", // textual
    "field_declaration",    // function declaration or field declaration in type
    "template_declaration", // decl or definition // class or function
    "friend_declaration",   // function decl, class // 获取identifier语义
    "function_definition",  // inline namespace and function definition
};

std::unordered_set<std::string> GraphBuilder::ComplexTypes = {
    "class_specifier",       "struct_specifier",
    "union_specifier",       "enum_specifier",
    "linkage_specification", // declaration or composite type
};

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
  // 如果是头文件，并且使用的是传统的header
  // guard，那么TSRoot是一个preproc_ifdef，
  // 并且FrontDeclCnt同时也要进行更新；如果是头文件或实现文件，有#pragma header
  // guard 或无header guard，那么TSRoot是translation_unit; FrontDeclCnt不用更新
  std::shared_ptr<SemanticNode> TUPtr = parseTranslationUnit(
      TSRoot, IsConflicting, Path, FilePath, FrontDeclCnt, TURoot);
  vertex_descriptor TUVertex = addVertex(TUPtr);
  parseCompositeNode(TUPtr, TUVertex, IsConflicting, TURoot, FilePath,
                     FrontDeclCnt);

  // build edges

  // generate context info

  Client.DidClose(FilePath);
  if (HasAltFile) {
    Client.DidClose(AltUri);
  }
}

void GraphBuilder::parseCompositeNode(std::shared_ptr<SemanticNode> &SRoot,
                                      const vertex_descriptor &SRootVDesc,
                                      bool IsConflicting, const ts::Node &Root,
                                      const std::string &FilePath,
                                      const int FirstChildIdx /* = 0 */) {
  namespace symbols = ts::cpp::symbols;
  namespace fields = ts::cpp::fields;
  size_t ChildCnt = 0;
  for (size_t Idx = FirstChildIdx; Idx < Root.childrenCount(); ++ChildCnt) {
    const ts::Node &Child = Root.children[Idx];
    const std::string ChildType = Child.type();
    if (CompositeTypes.count(ChildType)) { // plain Composite
      std::shared_ptr<SemanticNode> NamespacePtr =
          parseNamespaceNode(Child, IsConflicting, FilePath);
      auto [CurDesc, _] =
          insertToGraphAndParent(SRoot, SRootVDesc, NamespacePtr);
      parseCompositeNode(
          NamespacePtr, CurDesc, IsConflicting,
          Child.getChildByFieldName(fields::field_body.name).value(), FilePath);
    } else if (TerminalTypes.count(ChildType)) { // plain terminal
      if (details::IsTextualNode(ChildType)) {
        std::shared_ptr<SemanticNode> TextualPtr =
            parseTextualNode(Child, IsConflicting, SRoot->hashSignature());
        TextualPtr->FollowingEOL = ts::getFollowingEOLs(Child);
        insertToGraphAndParent(SRoot, SRootVDesc, TextualPtr);
      } else if (ChildType == symbols::sym_comment.name) {
        size_t CommentCnt = 0;
        auto [Orphan, Comment] = ts::getComment(Child, CommentCnt);
        if (Orphan) {
          std::shared_ptr<SemanticNode> OrphanCommentPtr =
              std::make_shared<OrphanCommentNode>(
                  NodeCount++, IsConflicting, NodeKind::ORPHAN_COMMENT, Comment,
                  "", Comment, "", Child.startPoint(), "", "",
                  ts::getFollowingEOLs(Child));
          insertToGraphAndParent(SRoot, SRootVDesc, OrphanCommentPtr);
          Idx +=
              CommentCnt - 1; // -1 as the loop end will auto increment Idx by 1
        }
      } else if (ChildType == symbols::sym_field_declaration.name) {
        std::shared_ptr<SemanticNode> FieldDeclPtr = parseFieldDeclarationNode(
            Child, IsConflicting, SRoot->hashSignature(), FilePath);
        FieldDeclPtr->FollowingEOL = ts::getFollowingEOLs(Child);
        insertToGraphAndParent(SRoot, SRootVDesc, FieldDeclPtr);
      } else if (ChildType == symbols::sym_function_definition.name) {
        const std::optional<ts::Node> TypeOpt =
            Child.getChildByFieldName(fields::field_type.name);
        if (TypeOpt.has_value()) { // with return value
          if (TypeOpt.value().text() == "namespace") {
            // tree-sitter will parse inline namespace as function definition
            std::shared_ptr<SemanticNode> NamespacePtr =
                parseNamespaceNode(Child, IsConflicting, FilePath);
            auto [NSDesc, _] =
                insertToGraphAndParent(SRoot, SRootVDesc, NamespacePtr);
            parseCompositeNode(
                NamespacePtr, NSDesc, IsConflicting,
                Child.getChildByFieldName(fields::field_body.name).value(),
                FilePath);
          } else {
            // plain function definition
            std::shared_ptr<SemanticNode> FncDefNodePtr =
                parseFuncDefNode(Child, IsConflicting, FilePath);
            insertToGraphAndParent(SRoot, SRootVDesc, FncDefNodePtr);
          }
        } else { // without return value
          const std::optional<ts::Node> declaratorOpt =
              Child.getChildByFieldName(fields::field_declarator.name);
          if (declaratorOpt.has_value() &&
              declaratorOpt.value().type() ==
                  symbols::sym_operator_cast.name) { // operator cast function
            std::shared_ptr<SemanticNode> FuncOperatorCastNodePtr =
                parseFuncOperatorCastNode(Child, IsConflicting, FilePath);
            insertToGraphAndParent(SRoot, SRootVDesc, FuncOperatorCastNodePtr);
          } else { // special function member
            std::shared_ptr<SemanticNode> FuncSpecialMemberNodePtr =
                parseFuncSpecialMemberNode(Child, IsConflicting, FilePath);
            insertToGraphAndParent(SRoot, SRootVDesc, FuncSpecialMemberNodePtr);
          }
        }
      } else if (ChildType == symbols::sym_template_declaration.name) {
        // class template or function template definition
        enum TemplateDeclKind { CLASS_TEMPLATE, FUNCTION_TEMPLATE, UNKNOWN };
        TemplateDeclKind Kind = UNKNOWN;
        for (const auto &ChildChild : Child.children) {
          if (details::IsClassSpecifier(ChildChild.type())) {
            Kind = CLASS_TEMPLATE;
            break;
          } else if (ChildChild.type() ==
                     symbols::sym_function_definition.name) {
            Kind = FUNCTION_TEMPLATE;
            break;
          }
        }
        if (Kind == FUNCTION_TEMPLATE) {
          std::shared_ptr<SemanticNode> FuncDefNodePtr =
              parseFuncDefNode(Child, IsConflicting, FilePath);
          insertToGraphAndParent(SRoot, SRootVDesc, FuncDefNodePtr);
        } else if (Kind == CLASS_TEMPLATE) {
          auto [TypeDeclNodePtr, TypeKind] =
              parseTypeDeclNode(Child, IsConflicting, FilePath);
          std::shared_ptr<SemanticNode> SemanticPtr = TypeDeclNodePtr;
          auto [TypeDeclDesc, _] =
              insertToGraphAndParent(SRoot, SRootVDesc, SemanticPtr);
          parseCompositeNode(
              SemanticPtr, TypeDeclDesc, IsConflicting,
              Child.getChildByFieldName(fields::field_body.name).value(),
              FilePath);
          TypeDeclNodePtr->setMemberAccessSpecifier();
        } else {
          spdlog::error("unexpected template declaration: file path is {}, row "
                        "is: {}, node type is {}, isNamed is {}, text is {}",
                        FilePath, Child.startPoint(), Child.type(),
                        Child.isNamed(), Child.text());
          assert(false);
        }
      } else if (ChildType == symbols::sym_access_specifier.name) {
        SemanticNode *ParentRawPtr = SRoot.get();
        AccessSpecifierKind AccessKind =
            details::getAccessSpecifierKind(Child.text());
        // SRoot must be a class specifier type
        if (ChildCnt != 0) {
          if (auto *TypeDeclPtr = llvm::dyn_cast<TypeDeclNode>(ParentRawPtr)) {
            TypeDeclPtr->AccessSpecifierQue.push({AccessKind, ChildCnt});
          } else {
            spdlog::error("access specifier should only appear in type decl "
                          "node: file path is {}, location "
                          "is: {}, node type is {}, isNamed is {}, text is {}",
                          FilePath, Child.startPoint(), Child.type(),
                          Child.isNamed(), Child.text());
            assert(false &&
                   "access specifier should only appear in type decl node");
          }
        }
      }
    } else if (ComplexTypes.count(ChildType)) { // complex type
      if (details::IsClassSpecifier(ChildType)) {
        const std::optional<ts::Node> TypeBodyOpt = Child.getChildByFieldName(
            fields::field_body.name); // class, struct, union body
        if (!TypeBodyOpt.has_value()) {
          std::shared_ptr<TextualNode> TextualPtr =
              parseTextualNode(Child, IsConflicting, SRoot->hashSignature());
          insertToGraphAndParent(SRoot, SRootVDesc, TextualPtr);
        } else {
          auto [TypeDeclNodePtr, Kind] =
              parseTypeDeclNode(Child, IsConflicting, FilePath);
          std::shared_ptr<SemanticNode> SemanticPtr = TypeDeclNodePtr;
          auto [TypeDeclDesc, _] =
              insertToGraphAndParent(SRoot, SRootVDesc, SemanticPtr);
          parseCompositeNode(
              SemanticPtr, TypeDeclDesc, IsConflicting,
              Child.getChildByFieldName(fields::field_body.name).value(),
              FilePath);
          TypeDeclNodePtr->setMemberAccessSpecifier();
        }
      } else if (ChildType == symbols::sym_linkage_specification.name) {
        const std::optional<ts::Node> LinkageBodyOpt =
            Child.getChildByFieldName(fields::field_body.name);
        assert(LinkageBodyOpt.has_value() &&
               "linkage specification should have a body");
        const ts::Node &LinkageBody = LinkageBodyOpt.value();
        if (LinkageBody.type() == symbols::sym_declaration.name ||
            LinkageBody.type() == symbols::sym_function_definition.name) {
          std::shared_ptr<SemanticNode> TextualPtr =
              parseTextualNode(Child, IsConflicting, SRoot->hashSignature());
          insertToGraphAndParent(SRoot, SRootVDesc, TextualPtr);
        } else {
          std::shared_ptr<SemanticNode> LinkagePtr = parseLinkageSpecNode(
              Child, IsConflicting, SRoot->hashSignature());
          auto [LinkageDesc, _] =
              insertToGraphAndParent(SRoot, SRootVDesc, LinkagePtr);
          parseCompositeNode(
              LinkagePtr, LinkageDesc, IsConflicting,
              Child.getChildByFieldName(fields::field_body.name).value(),
              FilePath);
        }
      } else if (ChildType == symbols::sym_enum_specifier.name) {
        const std::optional<ts::Node> EnumBodyOpt =
            Child.getChildByFieldName(fields::field_body.name);
        if (!EnumBodyOpt.has_value()) { // empty enum specifier
          std::shared_ptr<TextualNode> TextualPtr =
              parseTextualNode(Child, IsConflicting, SRoot->hashSignature());
          insertToGraphAndParent(SRoot, SRootVDesc, TextualPtr);
        } else {
          std::shared_ptr<SemanticNode> EnumNodePtr =
              parseEnumNode(Child, IsConflicting, FilePath);
          auto [EnumDesc, _] =
              insertToGraphAndParent(SRoot, SRootVDesc, EnumNodePtr);
          parseCompositeNode(
              EnumNodePtr, EnumDesc, IsConflicting,
              Child.getChildByFieldName(fields::field_body.name).value(),
              FilePath);
        }
      }
    } else {
      spdlog::error(
          "unexpected node: file path is {}, location is: {}, node type is {}, "
          "isNamed is {}, text is {}",
          FilePath, Child.startPoint(), Child.type(), Child.isNamed(),
          Child.text());
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
      NodeCount++, IsConflicting, NodeKind::TRANSLATION_UNIT, Path,
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
  std::string Inline;
  int NOffset = 0;
  RE2 pattern(R"(((inline\s+)?namespace\s*([^\s{]*))\s*\{)");
  re2::StringPiece input(Node.text().data(), Node.text().size());
  if (RE2::PartialMatch(input, pattern, &OriginalSignature, &Inline,
                        &DisplayName)) {
    if (!DisplayName.empty()) {
      NOffset = input.find(DisplayName);
      size_t QualifiedOffset = DisplayName.rfind("::");
      if (QualifiedOffset != std::string::npos) {
        NOffset += (QualifiedOffset + 2); // 2 for "::"
      }
    }
  } else {
    assert(false && "it seems that Node is not a namespace node");
  }

  std::string USR;           // symbol details获取USR
  std::string QualifiedName; // symbol details 获取QualifiedName
  int row = Node.startPoint().row;
  int col = Node.startPoint().column;
  col += NOffset;
  std::optional<lsp::SymbolDetails> detailsOpt =
      getSymbolDetails(Path, lsp::Position{row, col});
  if (detailsOpt.has_value()) {
    const lsp::SymbolDetails &details = detailsOpt.value();
    USR = details.usr;
    QualifiedName = details.containerName.value_or("");
  }

  std::string Comment = getNodeComment(Node);
  size_t BeforeFirstChildEOL = ts::beforeFirstChildEOLs(Node);

  std::string NSComment;
  if (Node.nextSibling().has_value() &&
      Node.nextSibling().value().type() == ts::cpp::symbols::sym_comment.name) {
    NSComment = Node.nextSibling().value().text();
  }

  return std::make_shared<NamespaceNode>(
      NodeCount++, IsConflicting, NodeKind::NAMESPACE, DisplayName,
      QualifiedName, OriginalSignature, std::move(Comment), Node.startPoint(),
      std::move(USR), BeforeFirstChildEOL, Inline.size() != 0,
      std::move(NSComment));
}

std::shared_ptr<TextualNode>
GraphBuilder::parseTextualNode(const ts::Node &Node, bool IsConflicting,
                               size_t ParentSignatureHash) {
  std::string TextContent = Node.text();
  return std::make_shared<TextualNode>(
      NodeCount++, IsConflicting, NodeKind::TEXTUAL, TextContent, "",
      TextContent, ts::getNodeComment(Node), Node.startPoint(), "",
      std::move(TextContent), ts::getFollowingEOLs(Node), ParentSignatureHash);
}

std::shared_ptr<FieldDeclarationNode> GraphBuilder::parseFieldDeclarationNode(
    const ts::Node &Node, bool IsConflicting, size_t ParentSignatureHash,
    const std::string &FilePath) {
  const std::optional<ts::Node> DeclaratorNodeOpt =
      Node.getChildByFieldName(ts::cpp::fields::field_declarator.name);

  auto [RowPos, ColPos] = Node.startPoint();
  std::string Declarator;
  std::vector<std::string> References;
  if (DeclaratorNodeOpt.has_value()) {
    const ts::Node &DeclaratorNode = DeclaratorNodeOpt.value();
    auto StartPoint = DeclaratorNode.startPoint();
    RowPos = StartPoint.row;
    ColPos = StartPoint.column;

    // 查找References
    References = getReferences(
        FilePath, {static_cast<int>(RowPos), static_cast<int>(ColPos)});
  }

  std::string QualifiedName;
  std::string USR;
  std::string DisplayName;
  std::optional<lsp::SymbolDetails> detailsOpt =
      getSymbolDetails(FilePath, lsp::Position{static_cast<int>(RowPos),
                                               static_cast<int>(ColPos)});
  if (detailsOpt.has_value()) {
    const lsp::SymbolDetails &details = detailsOpt.value();
    USR = details.usr;
    QualifiedName = details.containerName.value_or("");
    DisplayName = details.name;
  }
  std::shared_ptr<FieldDeclarationNode> FDNodePtr =
      std::make_shared<FieldDeclarationNode>(
          NodeCount++, IsConflicting, NodeKind::FIELD_DECLARATION, DisplayName,
          QualifiedName, Node.text(), ts::getNodeComment(Node),
          Node.startPoint(), std::move(USR), Node.text(),
          ts::getFollowingEOLs(Node), std::move(Declarator),
          ParentSignatureHash);
}

std::shared_ptr<LinkageSpecNode>
GraphBuilder::parseLinkageSpecNode(const ts::Node &Node, bool IsConflicting,
                                   size_t ParentSignatureHash) {
  return std::make_shared<LinkageSpecNode>(
      NodeCount++, IsConflicting, NodeKind::LINKAGE_SPEC_LIST,
      "extern \"C\" {}", "", "", ts::getNodeComment(Node), Node.startPoint(),
      "", ts::beforeFirstChildEOLs(Node), ParentSignatureHash);
}

std::shared_ptr<EnumNode>
GraphBuilder::parseEnumNode(const ts::Node &Node, bool IsConflicting,
                            const std::string &FilePath) {
  auto [row, col] = Node.startPoint();

  const std::string pattern =
      R"(((enum\s*(class|struct)?)((\s*\[\[[^\]]+\]\])*)?\s*([a-zA-Z_][a-zA-Z0-9_:]*)?\s*(:\s*[a-zA-Z_][a-zA-Z0-9_]*)?)\s*\{)";
  re2::RE2 re(pattern);
  std::string EnumKey, Attrs, EnumName, EnumBase, OriginalSignature;
  re2::StringPiece input(Node.text());
  re2::StringPiece enum_name_pieces;
  if (re2::RE2::PartialMatch(input, re, &OriginalSignature, &EnumKey, nullptr,
                             &Attrs, nullptr, &enum_name_pieces, &EnumBase)) {
    EnumName = enum_name_pieces.ToString();
    if (EnumName.back() == ' ') {
      EnumName.pop_back();
    }
    // 获取最后一个层级名称的位置
    size_t last_colon = enum_name_pieces.rfind("::");
    size_t offset_from_start =
        enum_name_pieces.empty() ? 0
        : last_colon == std::string::npos
            ? enum_name_pieces.data() - input.data()
            : enum_name_pieces.data() - input.data() + last_colon + 2;
    col += offset_from_start;
  } else {
    assert(false && "it seems that Node is not an enum node");
  }

  std::string QualifiedName;
  std::string USR;
  std::optional<lsp::SymbolDetails> detailsOpt = getSymbolDetails(
      FilePath, lsp::Position{static_cast<int>(row), static_cast<int>(col)});
  if (detailsOpt.has_value()) {
    const lsp::SymbolDetails &details = detailsOpt.value();
    USR = details.usr;
    QualifiedName = details.containerName.value_or("");
  }
  return std::make_shared<EnumNode>(
      NodeCount++, IsConflicting, NodeKind::ENUM, EnumName, QualifiedName,
      OriginalSignature, ts::getNodeComment(Node), Node.startPoint(),
      std::move(USR), ts::beforeFirstChildEOLs(Node), EnumKey, Attrs, EnumBase);
}

std::pair<std::shared_ptr<TypeDeclNode>, TypeDeclNode::TypeDeclKind>
GraphBuilder::parseTypeDeclNode(const ts::Node &Node, bool IsConflicting,
                                const std::string &FilePath) {
  using TypeDeclKind = TypeDeclNode::TypeDeclKind;
  AccessSpecifierKind Access = AccessSpecifierKind::Default;
  const std::optional<ts::Node> BodyOpt = Node.getChildByFieldName(
      ts::cpp::fields::field_body.name); // class, struct, union body
  assert(BodyOpt.has_value() && "type declaration without body");
  const ts::Node &Body = BodyOpt.value();
  if (Body.childrenCount()) {
    const ts::Node &First = Body.children[0];
    if (First.type() == ts::cpp::symbols::sym_access_specifier.name) {
      Access = First.text() == "public"      ? AccessSpecifierKind::Public
               : First.text() == "protected" ? AccessSpecifierKind::Protected
                                             : AccessSpecifierKind::Private;
    } // else default visibility of type decl
  }

  ts::ClassInfo Info = ts::extractClassInfo(Node.text());

  TypeDeclKind Kind = Info.ClassKey == "class"    ? TypeDeclKind::Class
                      : Info.ClassKey == "struct" ? TypeDeclKind::Struct
                                                  : TypeDeclKind::Union;

  std::string QualifiedName;
  std::string USR;
  auto [row, col] = Node.startPoint();
  std::optional<lsp::SymbolDetails> detailsOpt = getSymbolDetails(
      FilePath, lsp::Position{static_cast<int>(row + Info.LineOffset),
                              static_cast<int>(col + Info.ColOffset)});
  if (detailsOpt.has_value()) {
    const lsp::SymbolDetails &details = detailsOpt.value();
    USR = details.usr;
    QualifiedName = details.containerName.value_or("");
  }

  return std::pair<std::shared_ptr<TypeDeclNode>, TypeDeclNode::TypeDeclKind>({
      std::make_shared<TypeDeclNode>(
          NodeCount++, IsConflicting, NodeKind::TYPE, Info.ClassName,
          QualifiedName, Info.OriginalSignature, ts::getNodeComment(Node),
          Node.startPoint(), std::move(USR), ts::beforeFirstChildEOLs(Node),
          Kind, std::move(Info.Attrs), Info.IsFinal, std::move(Info.BaseClause),
          Access, std::move(Info.TemplateParameterList)),
      Kind,
  });
}

std::shared_ptr<FuncDefNode>
GraphBuilder::parseFuncDefNode(const ts::Node &Node, bool IsConflicting,
                               const std::string &FilePath) {
  ts::Node FDefNode = Node;
  if (Node.type() == ts::cpp::symbols::sym_template_declaration.name) {
    for (size_t i = 0; i < Node.childrenCount(); ++i) {
      if (Node.children[i].type() ==
          ts::cpp::symbols::sym_function_definition.name) {
        FDefNode = Node.children[i];
        break;
      }
    }
  }
  const ts::Node DeclaratorNode =
      FDefNode.getChildByFieldName(ts::cpp::fields::field_declarator.name)
          .value();
  std::string FuncName =
      DeclaratorNode.text().substr(0, DeclaratorNode.text().find('('));
  ts::FuncDefInfo DefInfo = ts::extractFuncDefInfo(FDefNode.text(), FuncName);

  std::string QualifiedName;
  std::string USR;
  auto [row, col] = Node.startPoint();
  std::optional<lsp::SymbolDetails> detailsOpt = getSymbolDetails(
      FilePath, lsp::Position{static_cast<int>(row + DefInfo.LineOffset),
                              static_cast<int>(col + DefInfo.ColOffset)});
  if (detailsOpt.has_value()) {
    const lsp::SymbolDetails &details = detailsOpt.value();
    USR = details.usr;
    QualifiedName = details.containerName.value_or("");
  }

  std::vector<std::string> ParameterTypeList;
  if (USR.size()) {
    size_t Pos = USR.find_first_of('$');
    if (Pos != std::string::npos) { // function has parameters
      std::string TypeNotationPattern = "@S|@U|@E";
      re2::RE2 TypeNotationRe(TypeNotationPattern);
      bool HasTypeNotation = re2::RE2::PartialMatch(USR, TypeNotationRe);
      if (HasTypeNotation) {
        std::string ParameterTypes = USR.substr(Pos);
        std::vector<std::string_view> ParameterInfos =
            util::string_split(ParameterTypes, "$", false);
        std::string pattern = R"(^([@a-zA-Z]+))";
        re2::RE2 re(pattern);
        for (const auto ParameterInfo : ParameterInfos) {
          re2::StringPiece ParameterInfo_sp(ParameterInfo);
          re2::StringPiece ParameterType;
          if (re2::RE2::PartialMatch(ParameterInfo_sp, re, &ParameterType)) {
            if (ParameterType.starts_with("@N@") ||
                ParameterType.starts_with("@S@") ||
                ParameterType.starts_with("@U@")) { // remove top-level ::
              ParameterType.remove_prefix(3);
            }
            std::string QualifiedParamType = ParameterType.ToString();
            re2::RE2::GlobalReplace(&QualifiedParamType, "@N@|@S@|@E@", "::");
            ParameterTypeList.push_back(QualifiedParamType);
          }
        }
      }
    }
  }

  const std::optional<ts::Node> BodyOpt = Node.getChildByFieldName(
      ts::cpp::fields::field_body.name); // function body
  std::string BodyText = BodyOpt.has_value() ? BodyOpt.value().text() : "";

  std::shared_ptr<FuncDefNode> FuncDefNodePtr = std::make_shared<FuncDefNode>(
      NodeCount++, IsConflicting, NodeKind::FUNC_DEF, DefInfo.FuncName,
      QualifiedName, DefInfo.OriginalSignature, ts::getNodeComment(Node),
      Node.startPoint(), std::move(USR), std::move(BodyText),
      ts::getFollowingEOLs(Node), std::move(DefInfo.TemplateParameterList),
      std::move(DefInfo.Attrs), std::move(DefInfo.BeforeFuncName),
      std::move(DefInfo.ParameterList), std::move(DefInfo.AfterParameterList));

  FuncDefNodePtr->ParameterTypes = std::move(ParameterTypeList);

  return FuncDefNodePtr;
}

std::shared_ptr<FuncOperatorCastNode> GraphBuilder::parseFuncOperatorCastNode(
    const ts::Node &Node, bool IsConflicting, const std::string &FilePath) {
  ts::FuncOperatorCastInfo Info = ts::extractFuncOperatorCastInfo(Node.text());

  std::string QualifiedName;
  std::string USR;
  auto [row, col] = Node.startPoint();
  std::optional<lsp::SymbolDetails> detailsOpt = getSymbolDetails(
      FilePath, lsp::Position{static_cast<int>(row + Info.LineOffset),
                              static_cast<int>(col + Info.ColOffset)});
  if (detailsOpt.has_value()) {
    const lsp::SymbolDetails &details = detailsOpt.value();
    USR = details.usr;
    QualifiedName = details.containerName.value_or("");
  }

  const std::optional<ts::Node> BodyOpt = Node.getChildByFieldName(
      ts::cpp::fields::field_body.name); // function body
  std::string BodyText = BodyOpt.has_value() ? BodyOpt.value().text() : "";

  return std::make_shared<FuncOperatorCastNode>(
      NodeCount++, IsConflicting, NodeKind::FUNC_OPERATOR_CAST, Info.FuncName,
      QualifiedName, Info.OriginalSignature, ts::getNodeComment(Node),
      Node.startPoint(), std::move(USR), std::move(BodyText),
      ts::getFollowingEOLs(Node), std::move(Info.TemplateParameterList),
      std::move(Info.Attrs), std::move(Info.ParameterList),
      std::move(Info.AfterParameterList));
}

std::shared_ptr<FuncSpecialMemberNode> GraphBuilder::parseFuncSpecialMemberNode(
    const ts::Node &Node, bool IsConflicting, const std::string &FilePath) {
  ts::FuncSpecialMemberInfo Info =
      ts::extractFuncSpecialMemberInfo(Node.text());

  std::string QualifiedName;
  std::string USR;
  auto [row, col] = Node.startPoint();
  std::optional<lsp::SymbolDetails> detailsOpt = getSymbolDetails(
      FilePath, lsp::Position{static_cast<int>(row + Info.LineOffset),
                              static_cast<int>(col + Info.ColOffset)});
  if (detailsOpt.has_value()) {
    const lsp::SymbolDetails &details = detailsOpt.value();
    USR = details.usr;
    QualifiedName = details.containerName.value_or("");
  }

  std::vector<std::string> ParameterTypeList;
  if (USR.size()) {
    size_t Pos = USR.find_first_of('$');
    if (Pos != std::string::npos) { // function has parameters
      std::string TypeNotationPattern = "@S|@U|@E";
      re2::RE2 TypeNotationRe(TypeNotationPattern);
      bool HasTypeNotation = re2::RE2::PartialMatch(USR, TypeNotationRe);
      if (HasTypeNotation) {
        std::string ParameterTypes = USR.substr(Pos);
        std::vector<std::string_view> ParameterInfos =
            util::string_split(ParameterTypes, "$", false);
        std::string pattern = R"(^([@a-zA-Z]+))";
        re2::RE2 re(pattern);
        for (const auto ParameterInfo : ParameterInfos) {
          re2::StringPiece ParameterInfo_sp(ParameterInfo);
          re2::StringPiece ParameterType;
          if (re2::RE2::PartialMatch(ParameterInfo_sp, re, &ParameterType)) {
            if (ParameterType.starts_with("@N@") ||
                ParameterType.starts_with("@S@") ||
                ParameterType.starts_with("@U@")) { // remove top-level ::
              ParameterType.remove_prefix(3);
            }
            std::string QualifiedParamType = ParameterType.ToString();
            re2::RE2::GlobalReplace(&QualifiedParamType, "@N@|@S@|@E@", "::");
            ParameterTypeList.push_back(QualifiedParamType);
          }
        }
      }
    }
  }

  const std::optional<ts::Node> BodyOpt = Node.getChildByFieldName(
      ts::cpp::fields::field_body.name); // function body
  std::string BodyText = BodyOpt.has_value() ? BodyOpt.value().text() : "";

  std::shared_ptr<FuncSpecialMemberNode> FSMPtr =
      std::make_shared<FuncSpecialMemberNode>(
          NodeCount++, IsConflicting, NodeKind::FUNC_SPECIAL_MEMBER,
          Info.FuncName, QualifiedName, Info.OriginalSignature,
          ts::getNodeComment(Node), Node.startPoint(), std::move(USR),
          std::move(BodyText), ts::getFollowingEOLs(Node), Info.DefType,
          std::move(Info.TemplateParameterList), std::move(Info.Attrs),
          std::move(Info.BeforeFuncName), std::move(Info.ParameterList),
          std::move(Info.InitList));
  FSMPtr->ParameterTypes = std::move(ParameterTypeList);

  return FSMPtr;
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

std::vector<std::string> GraphBuilder::getReferences(const lsp::URIForFile &URI,
                                                     const lsp::Position Pos) {
  auto returned = Client.References(URI, Pos);
  if (!returned.has_value()) {
    return {};
  }

  std::vector<lsp::ReferenceLocation> locations = returned.value();
  if (!locations.size()) {
    return {};
  }

  std::vector<std::string> References;
  for (auto &location : locations) {
    if (location.containerName.has_value()) {
      References.push_back(std::move(location.containerName.value()));
    }
  }
  return References;
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
  // Note(hwa): will there be a memory leak?
  return boost::add_vertex(Node, G);
}

std::pair<GraphBuilder::edge_descriptor, bool>
GraphBuilder::addEdge(GraphBuilder::vertex_descriptor Source,
                      GraphBuilder::vertex_descriptor Target,
                      SemanticEdge Edge) {
  return boost::add_edge(Source, Target, Edge, G);
}

std::pair<GraphBuilder::vertex_descriptor, bool>
GraphBuilder::insertToGraphAndParent(
    std::shared_ptr<SemanticNode> &ParentPtr,
    const vertex_descriptor &ParentDesc,
    const std::shared_ptr<SemanticNode> &CurPtr) {
  vertex_descriptor CurDesc = addVertex(CurPtr);
  auto [_, Success] = addEdge(ParentDesc, CurDesc,
                              SemanticEdge(EdgeCount++, EdgeKind::CONTAIN));
  ParentPtr->Children.push_back(CurPtr);
  return {CurDesc, Success};
}

GraphBuilder::~GraphBuilder() {
  Client.Shutdown();
  Client.Exit();
}

// std::shared_ptr<IfDefBlockNode>
// GraphBuilder::parseIfDefBlockNode(const ts::Node &Node, bool IsConflicting) {
//   std::string TextContent = Node.text();
//   std::string DisplayName = TextContent;
//   std::size_t Pos = TextContent.find('\n');
//   if (Pos != std::string::npos) {
//     DisplayName = TextContent.substr(0, Pos);
//   }
//   return std::make_shared<IfDefBlockNode>(
//       NodeCount++, IsConflicting, NodeKind::IFDEF_BLOCK, DisplayName, "",
//       TextContent, ts::getNodeComment(Node), Node.startPoint(), "",
//       std::move(TextContent));
// }

// std::shared_ptr<AliasNode> GraphBuilder::parseAliasNode(const ts::Node &Node,
//                                                         bool IsConflicting) {
//   if (Node.type() == ts::cpp::symbols::sym_alias_declaration.name) {
////    RE2::FullMatch(code, "using\\s+(\\w+)\\s*=\\s*(.+)\\s*;", &key, &value)
//    std::string Key;
//    std::string Value;
//    return std::make_shared<AliasNode>(
//        NodeCount++, IsConflicting, NodeKind::USING, Node.text(), "",
//        Node.text(), ts::getNodeComment(Node), Node.startPoint(), "",
//        Node.text(), false, Key);
//  } else {
//    //    RE2::FullMatch(code, "typedef\\s+(.+)\\s+(\\w+)\\s*;", &value, &key)
//  }
//}
} // namespace sa
} // namespace mergebot