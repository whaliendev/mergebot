//
// Created by whalien on 11/06/23.
//

#ifndef MB_INCLUDE_MERGEBOT_LSP_PROTOCOL_H
#define MB_INCLUDE_MERGEBOT_LSP_PROTOCOL_H

#include <magic_enum.hpp>
#include <nlohmann/json.hpp>

#include "mergebot/utils/stringop.h"
#include "uri.h"

#define MAP_JSON(...) \
  { j = {__VA_ARGS__}; }
#define MAP_KEY(KEY) \
  { #KEY, value.KEY }
#define MAP_TO(KEY, TO) \
  { KEY, value.TO }
#define MAP_KV(K, ...) \
  {                    \
    K, { __VA_ARGS__ } \
  }
#define FROM_KEY(KEY) \
  if (j.contains(#KEY)) j.at(#KEY).get_to(value.KEY);
#define JSON_SERIALIZE(Type, TO, FROM)                         \
  template <>                                                  \
  struct adl_serializer<Type> {                                \
    static void to_json(json& j, const Type& value) TO         \
        static void from_json(const json& j, Type& value) FROM \
  };

namespace mergebot {
namespace lsp {
using mergebot::lsp::URIForFile;
using json = nlohmann::json;
using TextType = std::string;

enum class ErrorCode {
  // Defined by JSON RPC.
  ParseError = -32700,
  InvalidRequest = -32600,
  MethodNotFound = -32601,
  InvalidParams = -32602,
  InternalError = -32603,

  ServerNotInitialized = -32002,
  UnknownErrorCode = -32001,

  // Defined by the protocol.
  RequestCancelled = -32800,
  ContentModified = -32801,
};

class LSPError {
 public:
  std::string message;
  ErrorCode code;
  static char id;

  LSPError(const std::string& message, ErrorCode code)
      : message(message), code(code) {}

  std::string toString() const {
    std::ostringstream oss;
    oss << *this;
    return oss.str();
  }

  friend std::ostream& operator<<(std::ostream& os, const LSPError& error) {
    return os << fmt::format("LSPError(code={}, message={})",
                             magic_enum::enum_name(error.code), error.message);
  }
};

struct TextDocumentIdentifier {
  URIForFile uri;
};

struct VersionedTextDocumentIdentifier : public TextDocumentIdentifier {
  /// The version number of this document. If a versioned text document
  /// identifier is sent from the server to the client and the file is not open
  /// in the editor (the server has not received an open notification before)
  /// the server can send `null` to indicate that the version is known and the
  /// content on disk is the master (as speced with document content ownership).
  ///
  /// The version number of a document will increase after each change,
  /// including undo/redo. The number doesn't need to be consecutive.
  ///
  /// clangd extension: versions are optional, and synthesized if missing.
  std::optional<std::int64_t> version;
};

struct Position {
  /// Line position in a document (zero-based).
  int line = 0;

  /// Character offset on a line in a document (zero-based).
  /// WARNING: this is in UTF-16 codepoints, not bytes or characters!
  /// Use the functions in SourceCode.h to construct/interpret Positions.
  int character = 0;

  friend bool operator==(const Position& LHS, const Position& RHS) {
    return std::tie(LHS.line, LHS.character) ==
           std::tie(RHS.line, RHS.character);
  }
  friend bool operator!=(const Position& LHS, const Position& RHS) {
    return !(LHS == RHS);
  }
  friend bool operator<(const Position& LHS, const Position& RHS) {
    return std::tie(LHS.line, LHS.character) <
           std::tie(RHS.line, RHS.character);
  }
  friend bool operator<=(const Position& LHS, const Position& RHS) {
    return std::tie(LHS.line, LHS.character) <=
           std::tie(RHS.line, RHS.character);
  }

  friend std::ostream& operator<<(std::ostream& os, const Position& pos) {
    return os << pos.line << ':' << pos.character;
  }
};

struct Range {
  /// The range's start position.
  Position start;

  /// The range's end position.
  Position end;

  friend bool operator==(const Range& LHS, const Range& RHS) {
    return std::tie(LHS.start, LHS.end) == std::tie(RHS.start, RHS.end);
  }
  friend bool operator!=(const Range& LHS, const Range& RHS) {
    return !(LHS == RHS);
  }
  friend bool operator<(const Range& LHS, const Range& RHS) {
    return std::tie(LHS.start, LHS.end) < std::tie(RHS.start, RHS.end);
  }

  bool contains(Position Pos) const { return start <= Pos && Pos < end; }
  bool contains(Range Rng) const {
    return start <= Rng.start && Rng.end <= end;
  }

  friend std::ostream& operator<<(std::ostream& os, const Range& range) {
    return os << range.start << '-' << range.end;
  }
};

struct Location {
  /// The text document's URI.
  URIForFile uri;
  Range range;

  friend bool operator==(const Location& LHS, const Location& RHS) {
    return LHS.uri == RHS.uri && LHS.range == RHS.range;
  }

  friend bool operator!=(const Location& LHS, const Location& RHS) {
    return !(LHS == RHS);
  }

  friend bool operator<(const Location& LHS, const Location& RHS) {
    return std::tie(LHS.uri, LHS.range) < std::tie(RHS.uri, RHS.range);
  }
};

struct ReferenceLocation : Location {
  /// clangd extension: contains the name of the function or class in which the
  /// reference occurs
  std::optional<std::string> containerName;
};

struct TextEdit {
  /// The range of the text document to be manipulated. To insert
  /// text into a document create a range where start === end.
  Range range;

  /// The string to be inserted. For delete operations use an
  /// empty string.
  std::string newText;

  friend std::ostream& operator<<(std::ostream& os, const TextEdit& textEdit) {
    os << textEdit.range << " => \"";
    // escape string
    for (unsigned char ch : textEdit.newText) {
      if (ch == '\\') {
        os << '\\' << ch;
      } else if (isprint(ch) && ch != '"') {
        os << ch;
      } else {
        os << '\\' << util::hexdigit(ch >> 4) << util::hexdigit(ch & 0x0f);
      }
    }
    return os << '"';
  }
};
inline bool operator==(const TextEdit& L, const TextEdit& R) {
  return std::tie(L.newText, L.range) == std::tie(R.newText, R.range);
}

struct TextDocumentItem {
  /// The text document's URI.
  URIForFile uri;

  /// The text document's language identifier.
  std::string languageId;

  /// The version number of this document (it will strictly increase after each
  /// change, including undo/redo.
  ///
  /// clangd extension: versions are optional, and synthesized if missing.
  std::optional<int64_t> version;

  /// The content of the opened text document.
  std::string text;
};

enum class TraceLevel {
  Off = 0,
  Messages = 1,
  Verbose = 2,
};

enum class TextDocumentSyncKind {
  /// Documents should not be synced at all.
  None = 0,
  /// Documents are synced by always sending the full content of the document.
  Full = 1,
  /// Documents are synced by sending the full content on open.  After that
  /// only incremental updates to the document are send.
  Incremental = 2,
};

enum class CompletionItemKind {
  Missing = 0,
  Text = 1,
  Method = 2,
  Function = 3,
  Constructor = 4,
  Field = 5,
  Variable = 6,
  Class = 7,
  Interface = 8,
  Module = 9,
  Property = 10,
  Unit = 11,
  Value = 12,
  Enum = 13,
  Keyword = 14,
  Snippet = 15,
  Color = 16,
  File = 17,
  Reference = 18,
  Folder = 19,
  EnumMember = 20,
  Constant = 21,
  Struct = 22,
  Event = 23,
  Operator = 24,
  TypeParameter = 25,
};
constexpr auto CompletionItemKindMin =
    static_cast<size_t>(CompletionItemKind::Text);
constexpr auto CompletionItemKindMax =
    static_cast<size_t>(CompletionItemKind::TypeParameter);

enum class SymbolKind {
  File = 1,
  Module = 2,
  Namespace = 3,
  Package = 4,
  Class = 5,
  Method = 6,
  Property = 7,
  Field = 8,
  Constructor = 9,
  Enum = 10,
  Interface = 11,
  Function = 12,
  Variable = 13,
  Constant = 14,
  String = 15,
  Number = 16,
  Boolean = 17,
  Array = 18,
  Object = 19,
  Key = 20,
  Null = 21,
  EnumMember = 22,
  Struct = 23,
  Event = 24,
  Operator = 25,
  TypeParameter = 26
};
constexpr auto SymbolKindMin = static_cast<size_t>(SymbolKind::File);
constexpr auto SymbolKindMax = static_cast<size_t>(SymbolKind::TypeParameter);

enum class OffsetEncoding {
  // Any string is legal on the wire. Unrecognized encodings parse as this.
  UnsupportedEncoding,
  // Length counts code units of UTF-16 encoded text. (Standard LSP behavior).
  UTF16,
  // Length counts bytes of UTF-8 encoded text. (Clangd extension).
  UTF8,
  // Length counts codepoints in unicode text. (Clangd extension).
  UTF32,
};

// Describes the content type that a client supports in various result literals
// like `Hover`, `ParameterInfo` or `CompletionItem`.
enum class MarkupKind {
  PlainText,
  Markdown,
};

enum class ResourceOperationKind { Create, Rename, Delete };
enum class FailureHandlingKind {
  Abort,
  Transactional,
  Undo,
  TextOnlyTransactional
};

// This struct doesn't mirror LSP!
// The protocol defines deeply nested structures for client capabilities.
// Instead of mapping them all, this just parses out the bits we care about.
struct ClientCapabilities {
  /// The supported set of SymbolKinds for workspace/symbol.
  /// workspace.symbol.symbolKind.valueSet
  std::vector<SymbolKind> WorkspaceSymbolKinds;

  /// Whether the client accepts diagnostics with codeActions attached inline.
  /// textDocument.publishDiagnostics.codeActionsInline.
  bool DiagnosticFixes = false;

  /// Whether the client accepts diagnostics with related locations.
  /// textDocument.publishDiagnostics.relatedInformation.
  bool DiagnosticRelatedInformation = false;

  /// Whether the client accepts diagnostics with category attached to it
  /// using the "category" extension.
  /// textDocument.publishDiagnostics.categorySupport
  bool DiagnosticCategory = false;

  /// Client supports snippets as insert text.
  /// textDocument.completion.completionItem.snippetSupport
  bool CompletionSnippets = false;

  /// Client supports completions with additionalTextEdit near the cursor.
  /// This is a clangd extension. (LSP says this is for unrelated text only).
  /// textDocument.completion.editsNearCursor
  bool CompletionFixes = false;

  /// Client supports displaying a container string for results of
  /// textDocument/reference (clangd extension)
  bool ReferenceContainer = true;

  /// Client supports hierarchical document symbols.
  /// textDocument.documentSymbol.hierarchicalDocumentSymbolSupport
  bool HierarchicalDocumentSymbol = true;

  /// Client supports signature help.
  /// textDocument.signatureHelp
  bool HasSignatureHelp = true;

  /// Client signals that it only supports folding complete lines.
  /// Client will ignore specified `startCharacter` and `endCharacter`
  /// properties in a FoldingRange.
  /// textDocument.foldingRange.lineFoldingOnly
  bool LineFoldingOnly = false;

  /// Client supports processing label offsets instead of a simple label string.
  /// textDocument.signatureHelp.signatureInformation.parameterInformation.labelOffsetSupport
  bool OffsetsInSignatureHelp = false;

  /// The documentation format that should be used for
  /// textDocument/signatureHelp.
  /// textDocument.signatureHelp.signatureInformation.documentationFormat
  MarkupKind SignatureHelpDocumentationFormat = MarkupKind::PlainText;

  /// The supported set of CompletionItemKinds for textDocument/completion.
  /// textDocument.completion.completionItemKind.valueSet
  std::vector<CompletionItemKind> CompletionItemKinds;

  /// The documentation format that should be used for textDocument/completion.
  /// textDocument.completion.completionItem.documentationFormat
  MarkupKind CompletionDocumentationFormat = MarkupKind::PlainText;

  /// Client supports CodeAction return value for textDocument/codeAction.
  /// textDocument.codeAction.codeActionLiteralSupport.
  bool CodeActionStructure = false;

  /// Client advertises support for the semanticTokens feature.
  /// We support the textDocument/semanticTokens request in any case.
  /// textDocument.semanticTokens
  bool SemanticTokens = false;
  /// Client supports Theia semantic highlighting extension.
  /// https://github.com/microsoft/vscode-languageserver-node/pull/367
  /// clangd no longer supports this, we detect it just to log a warning.
  /// textDocument.semanticHighlightingCapabilities.semanticHighlighting
  bool TheiaSemanticHighlighting = false;

  /// Supported encodings for LSP character offsets. (clangd extension).
  std::optional<std::vector<OffsetEncoding>> offsetEncoding;

  /// The content format that should be used for Hover requests.
  /// textDocument.hover.contentEncoding
  MarkupKind HoverContentFormat = MarkupKind::PlainText;

  /// The client supports testing for validity of rename operations
  /// before execution.
  bool RenamePrepareSupport = false;

  /// The client supports progress notifications.
  /// window.workDoneProgress
  bool WorkDoneProgress = false;

  /// The client supports implicit $/progress work-done progress streams,
  /// without a preceding window/workDoneProgress/create.
  /// This is a clangd extension.
  /// window.implicitWorkDoneProgressCreate
  bool ImplicitProgressCreation = false;

  /// Whether the client claims to cancel stale requests.
  /// general.staleRequestSupport.cancel
  bool CancelsStaleRequests = false;

  /// Whether the client implementation supports a refresh request sent from the
  /// server to the client.
  bool SemanticTokenRefreshSupport = false;

  ClientCapabilities() {
    for (size_t i = SymbolKindMin; i <= SymbolKindMax; ++i) {
      WorkspaceSymbolKinds.push_back(static_cast<SymbolKind>(i));
    }
    for (size_t i = CompletionItemKindMax; i <= CompletionItemKindMax; ++i) {
      CompletionItemKinds.push_back(static_cast<CompletionItemKind>(i));
    }
    offsetEncoding = {OffsetEncoding::UTF8};
  }
};

struct ServerCapabilities {
  json capabilities;
  /**
   * Defines how text documents are synced. Is either a detailed structure
   * defining each notification or for backwards compatibility the
   * TextDocumentSyncKind number. If omitted it defaults to
   * `TextDocumentSyncKind.None`.
   */
  TextDocumentSyncKind textDocumentSync = TextDocumentSyncKind::None;
  bool resolveProvider = false;
  std::vector<std::string> executeCommands;
  std::vector<std::string> signatureHelpTrigger;
  std::vector<std::string> formattingTrigger;
  std::vector<std::string> completionTrigger;
  bool hasProvider(std::string& name) {
    if (capabilities.contains(name)) {
      if (capabilities[name].type() == json::value_t::boolean) {
        return capabilities["name"];
      }
    }
    return false;
  }
};

struct ClangdCompileCommand {
  TextType workingDirectory;
  std::vector<TextType> compilationCommand;
};

struct ConfigurationSettings {
  // Changes to the in-memory compilation database.
  // The key of the map is a file name.
  std::map<std::string, ClangdCompileCommand> compilationDatabaseChanges;
};

struct InitializationOptions {
  // What we can change through the didChangeConfiguration request, we can
  // also set through the initialize request (initializationOptions field).
  ConfigurationSettings configSettings;

  std::optional<TextType> compilationDatabasePath;
  // Additional flags to be included in the "fallback command" used when
  // the compilation database doesn't describe an opened file.
  // The command used will be approximately `clang $FILE $fallbackFlags`.
  std::vector<TextType> fallbackFlags;

  /// Clients supports show file status for textDocument/clangd.fileStatus.
  bool clangdFileStatus = false;
};

struct InitializeParams {
  unsigned processId = 0;
  ClientCapabilities capabilities;
  std::optional<URIForFile> rootUri;
  /// @deprecated in favour of rootUri
  std::optional<TextType> rootPath;
  InitializationOptions initializationOptions;
};

enum class MessageType {
  /// An error message.
  Error = 1,
  /// A warning message.
  Warning = 2,
  /// An information message.
  Info = 3,
  /// A log message.
  Log = 4,
};
struct ShowMessageParams {
  /// The message type.
  MessageType type = MessageType::Info;
  /// The actual message.
  std::string message;
};

struct Registration {
  /**
   * The id used to register the request. The id can be used to deregister
   * the request again.
   */
  TextType id;
  /**
   * The method / capability to register for.
   */
  TextType method;
};

struct RegistrationParams {
  std::vector<Registration> registrations;
};

struct UnregistrationParams {
  std::vector<Registration> unregisterations;
};

struct DidOpenTextDocumentParams {
  /// The document that was opened.
  TextDocumentItem textDocument;
};

struct DidCloseTextDocumentParams {
  /// The document that was closed.
  TextDocumentIdentifier textDocument;
};

struct TextDocumentContentChangeEvent {
  /// The range of the document that changed.
  std::optional<Range> range;

  /// The length of the range that got replaced.
  std::optional<int> rangeLength;
  /// The new text of the range/document.
  std::string text;
};

struct DidChangeTextDocumentParams {
  /// The document that did change. The version number points
  /// to the version after all provided content changes have
  /// been applied.
  TextDocumentIdentifier textDocument;

  /// The actual content changes.
  std::vector<TextDocumentContentChangeEvent> contentChanges;

  /// Forces diagnostics to be generated, or to not be generated, for this
  /// version of the file. If not set, diagnostics are eventually consistent:
  /// either they will be provided for this version or some subsequent one.
  /// This is a clangd extension.
  std::optional<bool> wantDiagnostics;
};

enum class FileChangeType {
  /// The file got created.
  Created = 1,
  /// The file got changed.
  Changed = 2,
  /// The file got deleted.
  Deleted = 3
};
struct FileEvent {
  /// The file's URI.
  URIForFile uri;
  /// The change type.
  FileChangeType type = FileChangeType::Created;
};

struct DidChangeWatchedFilesParams {
  /// The actual file events.
  std::vector<FileEvent> changes;
};

struct DidChangeConfigurationParams {
  ConfigurationSettings settings;
};

struct DocumentRangeFormattingParams {
  /// The document to format.
  TextDocumentIdentifier textDocument;

  /// The range to format
  Range range;
};

struct DocumentOnTypeFormattingParams {
  /// The document to format.
  TextDocumentIdentifier textDocument;

  /// The position at which this request was sent.
  Position position;

  /// The character that has been typed.
  TextType ch;
};

struct FoldingRangeParams {
  /// The document to format.
  TextDocumentIdentifier textDocument;
};

enum class FoldingRangeKind {
  Comment,
  Imports,
  Region,
};
NLOHMANN_JSON_SERIALIZE_ENUM(FoldingRangeKind,
                             {{FoldingRangeKind::Comment, "comment"},
                              {FoldingRangeKind::Imports, "imports"},
                              {FoldingRangeKind::Region, "region"}})

struct FoldingRange {
  /**
   * The zero-based line number from where the folded range starts.
   */
  int startLine;
  /**
   * The zero-based character offset from where the folded range starts.
   * If not defined, defaults to the length of the start line.
   */
  int startCharacter;
  /**
   * The zero-based line number where the folded range ends.
   */
  int endLine;
  /**
   * The zero-based character offset before the folded range ends.
   * If not defined, defaults to the length of the end line.
   */
  int endCharacter;

  FoldingRangeKind kind;
};

struct SelectionRangeParams {
  /// The document to format.
  TextDocumentIdentifier textDocument;
  std::vector<Position> positions;
};

struct SelectionRange {
  Range range;
  std::unique_ptr<SelectionRange> parent;
};

struct DocumentFormattingParams {
  /// The document to format.
  TextDocumentIdentifier textDocument;
};

struct DocumentSymbolParams {
  // The text document to find symbols in.
  TextDocumentIdentifier textDocument;
};

struct DiagnosticRelatedInformation {
  /// The location of this related diagnostic information.
  Location location;
  /// The message of this related diagnostic information.
  std::string message;
};
struct CodeAction;

struct Diagnostic {
  /// The range at which the message applies.
  Range range;

  /// The diagnostic's severity. Can be omitted. If omitted it is up to the
  /// client to interpret diagnostics as error, warning, info or hint.
  int severity = 0;

  /// The diagnostic's code. Can be omitted.
  std::string code;

  /// A human-readable string describing the source of this
  /// diagnostic, e.g. 'typescript' or 'super lint'.
  std::string source;

  /// The diagnostic's message.
  std::string message;

  /// An array of related diagnostic information, e.g. when symbol-names within
  /// a scope collide all definitions can be marked via this property.
  std::optional<std::vector<DiagnosticRelatedInformation>> relatedInformation;

  /// The diagnostic's category. Can be omitted.
  /// An LSP extension that's used to send the name of the category over to the
  /// client. The category typically describes the compilation stage during
  /// which the issue was produced, e.g. "Semantic Issue" or "Parse Issue".
  std::optional<std::string> category;

  /// Clangd extension: code actions related to this diagnostic.
  /// Only with capability textDocument.publishDiagnostics.codeActionsInline.
  /// (These actions can also be obtained using textDocument/codeAction).
  std::optional<std::vector<CodeAction>> codeActions;
};

struct PublishDiagnosticsParams {
  /**
   * The URI for which diagnostic information is reported.
   */
  std::string uri;
  /**
   * An array of diagnostic information items.
   */
  std::vector<Diagnostic> diagnostics;
};

struct CodeActionContext {
  /// An array of diagnostics.
  std::vector<Diagnostic> diagnostics;
};

struct CodeActionParams {
  /// The document in which the command was invoked.
  TextDocumentIdentifier textDocument;

  /// The range for which the command was invoked.
  Range range;

  /// Context carrying additional information.
  CodeActionContext context;
};

struct WorkspaceEdit {
  /// Holds changes to existing resources.
  std::optional<std::map<std::string, std::vector<TextEdit>>> changes;

  /// Note: "documentChanges" is not currently used because currently there is
  /// no support for versioned edits.
};

struct TweakArgs {
  /// A file provided by the client on a textDocument/codeAction request.
  std::string file;
  /// A selection provided by the client on a textDocument/codeAction request.
  Range selection;
  /// ID of the tweak that should be executed. Corresponds to Tweak::id().
  std::string tweakID;
};

struct ExecuteCommandParams {
  std::string command;
  // Arguments
  std::optional<WorkspaceEdit> workspaceEdit;
  std::optional<TweakArgs> tweakArgs;
};

struct LspCommand : public ExecuteCommandParams {
  std::string title;
};

struct CodeAction {
  /// A short, human-readable, title for this code action.
  std::string title;

  /// The kind of the code action.
  /// Used to filter code actions.
  std::optional<std::string> kind;
  /// The diagnostics that this code action resolves.
  std::optional<std::vector<Diagnostic>> diagnostics;

  /// The workspace edit this code action performs.
  std::optional<WorkspaceEdit> edit;

  /// A command this code action executes. If a code action provides an edit
  /// and a command, first the edit is executed and then the command.
  std::optional<LspCommand> command;
};

struct SymbolInformation {
  /// The name of this symbol.
  std::string name;
  /// The kind of this symbol.
  SymbolKind kind = SymbolKind::Class;
  /// The location of this symbol.
  Location location;
  /// The name of the symbol containing this symbol.
  std::string containerName;
};

struct SymbolDetails {
  TextType name;
  TextType containerName;
  /// Unified Symbol Resolution identifier
  /// This is an opaque string uniquely identifying a symbol.
  /// Unlike SymbolID, it is variable-length and somewhat human-readable.
  /// It is a common representation across several clang tools.
  /// (See USRGeneration.h)
  TextType usr;
  std::optional<TextType> id;
};

struct WorkspaceSymbolParams {
  /// A non-empty query string
  TextType query;
};

struct ApplyWorkspaceEditParams {
  WorkspaceEdit edit;
};

struct TextDocumentPositionParams {
  /// The text document.
  TextDocumentIdentifier textDocument;

  /// The position inside the text document.
  Position position;
};

enum class CompletionTriggerKind {
  /// Completion was triggered by typing an identifier (24x7 code
  /// complete), manual invocation (e.g Ctrl+Space) or via API.
  Invoked = 1,
  /// Completion was triggered by a trigger character specified by
  /// the `triggerCharacters` properties of the `CompletionRegistrationOptions`.
  TriggerCharacter = 2,
  /// Completion was re-triggered as the current completion list is incomplete.
  TriggerTriggerForIncompleteCompletions = 3
};
struct CompletionContext {
  /// How the completion was triggered.
  CompletionTriggerKind triggerKind = CompletionTriggerKind::Invoked;
  /// The trigger character (a single character) that has trigger code complete.
  /// Is undefined if `triggerKind !== CompletionTriggerKind.TriggerCharacter`
  std::optional<TextType> triggerCharacter;
};

struct CompletionParams : TextDocumentPositionParams {
  std::optional<CompletionContext> context;
};

struct MarkupContent {
  MarkupKind kind = MarkupKind::PlainText;
  std::string value;
};

struct Hover {
  /// The hover's content
  MarkupContent contents;

  /// An optional range is a range inside a text document
  /// that is used to visualize a hover, e.g. by changing the background color.
  std::optional<Range> range;
};

enum class InsertTextFormat {
  Missing = 0,
  /// The primary text to be inserted is treated as a plain string.
  PlainText = 1,
  /// The primary text to be inserted is treated as a snippet.
  ///
  /// A snippet can define tab stops and placeholders with `$1`, `$2`
  /// and `${3:foo}`. `$0` defines the final tab stop, it defaults to the end
  /// of the snippet. Placeholders with equal identifiers are linked, that is
  /// typing in one will update others too.
  ///
  /// See also:
  /// https//github.com/Microsoft/vscode/blob/master/src/vs/editor/contrib/snippet/common/snippet.md
  Snippet = 2,
};
struct CompletionItem {
  /// The label of this completion item. By default also the text that is
  /// inserted when selecting this completion.
  std::string label;

  /// The kind of this completion item. Based of the kind an icon is chosen by
  /// the editor.
  CompletionItemKind kind = CompletionItemKind::Missing;

  /// A human-readable string with additional information about this item, like
  /// type or symbol information.
  std::string detail;

  /// A human-readable string that represents a doc-comment.
  std::string documentation;

  /// A string that should be used when comparing this item with other items.
  /// When `falsy` the label is used.
  std::string sortText;

  /// A string that should be used when filtering a set of completion items.
  /// When `falsy` the label is used.
  std::string filterText;

  /// A string that should be inserted to a document when selecting this
  /// completion. When `falsy` the label is used.
  std::string insertText;

  /// The format of the insert text. The format applies to both the `insertText`
  /// property and the `newText` property of a provided `textEdit`.
  InsertTextFormat insertTextFormat = InsertTextFormat::Missing;

  /// An edit which is applied to a document when selecting this completion.
  /// When an edit is provided `insertText` is ignored.
  ///
  /// Note: The range of the edit must be a single line range and it must
  /// contain the position at which completion has been requested.
  TextEdit textEdit;

  /// An optional array of additional text edits that are applied when selecting
  /// this completion. Edits must not overlap with the main edit nor with
  /// themselves.
  std::vector<TextEdit> additionalTextEdits;

  /// Indicates if this item is deprecated.
  bool deprecated = false;

  /// The score that clangd calculates to rank the returned completions.
  /// This excludes the fuzzy-match between `filterText` and the partial word.
  /// This can be used to re-rank results as the user types, using client-side
  /// fuzzy-matching (that score should be multiplied with this one).
  /// This is a clangd extension.
  float score = 0.f;

  // TODO(krasimir): Add custom commitCharacters for some of the completion
  // items. For example, it makes sense to use () only for the functions.
  // TODO(krasimir): The following optional fields defined by the language
  // server protocol are unsupported:
  //
  // data?: any - A data entry field that is preserved on a completion item
  //              between a completion and a completion resolve request.
};

struct CompletionList {
  /// The list is not complete. Further typing should result in recomputing the
  /// list.
  bool isIncomplete = false;

  /// The completion items.
  std::vector<CompletionItem> items;
};

struct ParameterInformation {
  /// The label of this parameter. Ignored when labelOffsets is set.
  std::string labelString;

  /// Inclusive start and exclusive end offsets withing the containing signature
  /// label.
  /// Offsets are computed by lspLength(), which counts UTF-16 code units by
  /// default but that can be overriden, see its documentation for details.
  std::optional<std::pair<unsigned, unsigned>> labelOffsets;

  /// The documentation of this parameter. Optional.
  std::string documentation;
};

struct SignatureInformation {
  /// The label of this signature. Mandatory.
  std::string label;

  /// The documentation of this signature. Optional.
  std::string documentation;

  /// The parameters of this signature.
  std::vector<ParameterInformation> parameters;
};

struct SignatureHelp {
  /// The resulting signatures.
  std::vector<SignatureInformation> signatures;
  /// The active signature.
  int activeSignature = 0;
  /// The active parameter of the active signature.
  int activeParameter = 0;
  /// Position of the start of the argument list, including opening paren. e.g.
  /// foo("first arg",   "second arg",
  ///    ^-argListStart   ^-cursor
  /// This is a clangd-specific extension, it is only available via C++ API and
  /// not currently serialized for the LSP.
  Position argListStart;
};

struct RenameParams {
  /// The document that was opened.
  TextDocumentIdentifier textDocument;

  /// The position at which this request was sent.
  Position position;

  /// The new name of the symbol.
  std::string newName;
};

enum class DocumentHighlightKind { Text = 1, Read = 2, Write = 3 };

struct DocumentHighlight {
  /// The range this highlight applies to.
  Range range;
  /// The highlight kind, default is DocumentHighlightKind.Text.
  DocumentHighlightKind kind = DocumentHighlightKind::Text;
  friend bool operator<(const DocumentHighlight& LHS,
                        const DocumentHighlight& RHS) {
    int LHSKind = static_cast<int>(LHS.kind);
    int RHSKind = static_cast<int>(RHS.kind);
    return std::tie(LHS.range, LHSKind) < std::tie(RHS.range, RHSKind);
  }
  friend bool operator==(const DocumentHighlight& LHS,
                         const DocumentHighlight& RHS) {
    return LHS.kind == RHS.kind && LHS.range == RHS.range;
  }
};
enum class TypeHierarchyDirection { Children = 0, Parents = 1, Both = 2 };

struct TypeHierarchyParams : public TextDocumentPositionParams {
  /// The hierarchy levels to resolve. `0` indicates no level.
  int resolve = 0;

  /// The direction of the hierarchy levels to resolve.
  TypeHierarchyDirection direction = TypeHierarchyDirection::Parents;
};

struct TypeHierarchyItem {
  /// The human readable name of the hierarchy item.
  std::string name;

  /// Optional detail for the hierarchy item. It can be, for instance, the
  /// signature of a function or method.
  std::optional<std::string> detail;

  /// The kind of the hierarchy item. For instance, class or interface.
  SymbolKind kind;

  /// `true` if the hierarchy item is deprecated. Otherwise, `false`.
  bool deprecated;

  /// The URI of the text document where this type hierarchy item belongs to.
  URIForFile uri;

  /// The range enclosing this type hierarchy item not including
  /// leading/trailing whitespace but everything else like comments. This
  /// information is typically used to determine if the client's cursor is
  /// inside the type hierarch item to reveal in the symbol in the UI.
  Range range;

  /// The range that should be selected and revealed when this type hierarchy
  /// item is being picked, e.g. the name of a function. Must be contained by
  /// the `range`.
  Range selectionRange;

  /// If this type hierarchy item is resolved, it contains the direct parents.
  /// Could be empty if the item does not have direct parents. If not defined,
  /// the parents have not been resolved yet.
  std::optional<std::vector<TypeHierarchyItem>> parents;

  /// If this type hierarchy item is resolved, it contains the direct children
  /// of the current item. Could be empty if the item does not have any
  /// descendants. If not defined, the children have not been resolved.
  std::optional<std::vector<TypeHierarchyItem>> children;

  /// The protocol has a slot here for an optional 'data' filed, which can
  /// be used to identify a type hierarchy item in a resolve request. We don't
  /// need this (the item itself is sufficient to identify what to resolve)
  /// so don't declare it.
};

struct ReferenceParams : public TextDocumentPositionParams {
  // For now, no options like context.includeDeclaration are supported.
};

struct FileStatus {
  /// The text document's URI.
  URIForFile uri;
  /// The human-readable string presents the current state of the file, can be
  /// shown in the UI (e.g. status bar).
  TextType state;
  // FIXME(krasimir): add detail messages.
};
}  // namespace lsp
}  // namespace mergebot

// partial specialization (full specialization works too)
NLOHMANN_JSON_NAMESPACE_BEGIN
template <typename T>
struct adl_serializer<std::optional<T>> {
  static void to_json(json& j, const std::optional<T>& opt) {
    if (opt == std::nullopt) {
      j = nullptr;
    } else {
      j = *opt;  // this will call adl_serializer<T>::to_json which will
                 // find the free function to_json in T's namespace!
    }
  }

  static void from_json(const json& j, std::optional<T>& opt) {
    if (j.is_null()) {
      opt = std::nullopt;
    } else {
      opt = j.get<T>();  // same as above, but with
                         // adl_serializer<T>::from_json
    }
  }
};

JSON_SERIALIZE(
    mergebot::lsp::URIForFile, { j = value.uri(); },
    {
      auto str = j.get<std::string>();
      auto parsed = mergebot::lsp::URI::parse(str);
      assert(parsed && "illegal URI");
      auto uri = parsed.value();
      assert(uri.scheme() == "file" &&
             "workspace files can only have 'file' URI scheme");
      auto uriForFile =
          mergebot::lsp::URIForFile::fromURI(uri.toString(), /*hintPath=*/"");
      assert(uriForFile && "unresolvable URI");
      value = std::move(uriForFile.value());
    })

JSON_SERIALIZE(mergebot::lsp::TextDocumentIdentifier, MAP_JSON(MAP_KEY(uri)),
               {})

JSON_SERIALIZE(mergebot::lsp::VersionedTextDocumentIdentifier,
               MAP_JSON(MAP_KEY(uri), MAP_KEY(version)), {});

JSON_SERIALIZE(mergebot::lsp::Position,
               MAP_JSON(MAP_KEY(line), MAP_KEY(character)),
               {FROM_KEY(line) FROM_KEY(character)})

JSON_SERIALIZE(mergebot::lsp::Range, MAP_JSON(MAP_KEY(start), MAP_KEY(end)),
               {FROM_KEY(start) FROM_KEY(end)});

JSON_SERIALIZE(mergebot::lsp::Location, MAP_JSON(MAP_KEY(uri), MAP_KEY(range)),
               {FROM_KEY(uri) FROM_KEY(range)});

JSON_SERIALIZE(mergebot::lsp::ReferenceLocation,
               MAP_JSON(MAP_KEY(uri), MAP_KEY(range), MAP_KEY(containerName)),
               {FROM_KEY(uri) FROM_KEY(range) FROM_KEY(containerName)})

JSON_SERIALIZE(mergebot::lsp::TextEdit,
               MAP_JSON(MAP_KEY(range), MAP_KEY(newText)),
               {FROM_KEY(range) FROM_KEY(newText)})

JSON_SERIALIZE(mergebot::lsp::TextDocumentItem,
               MAP_JSON(MAP_KEY(uri), MAP_KEY(languageId), MAP_KEY(version),
                        MAP_KEY(text)),
               {FROM_KEY(uri) FROM_KEY(languageId) FROM_KEY(version)
                    FROM_KEY(text)})

JSON_SERIALIZE(
    mergebot::lsp::ClientCapabilities,
    MAP_JSON(
        MAP_KV(
            "textDocument",
            MAP_KV(
                "publishDiagnostics",  // PublishDiagnosticsClientCapabilities
                MAP_TO("categorySupport", DiagnosticCategory),
                MAP_TO("codeActionsInline", DiagnosticFixes),
                MAP_TO("relatedInformation", DiagnosticRelatedInformation), ),
            MAP_KV("completion",  // CompletionClientCapabilities
                   MAP_KV("completionItem",
                          MAP_TO("snippetSupport", CompletionSnippets)),
                   MAP_KV("completionItemKind",
                          MAP_TO("valueSet", CompletionItemKinds)),
                   MAP_TO("editsNearCursor", CompletionFixes)),
            MAP_KV("codeAction",
                   MAP_TO("codeActionLiteralSupport", CodeActionStructure)),
            MAP_KV("documentSymbol", MAP_TO("hierarchicalDocumentSymbolSupport",
                                            HierarchicalDocumentSymbol)),
            MAP_KV("hover",  // HoverClientCapabilities
                   MAP_TO("contentFormat", HoverContentFormat)),
            MAP_KV("signatureHelp",
                   MAP_KV("signatureInformation",
                          MAP_KV("parameterInformation",
                                 MAP_TO("labelOffsetSupport",
                                        OffsetsInSignatureHelp))))),
        MAP_KV("workspace",      // WorkspaceEditClientCapabilities
               MAP_KV("symbol",  // WorkspaceSymbolClientCapabilities
                      MAP_KV("symbolKind",
                             MAP_TO("valueSet", WorkspaceSymbolKinds)))),
        MAP_TO("offsetEncoding", offsetEncoding)),
    {});

JSON_SERIALIZE(mergebot::lsp::ServerCapabilities, {}, {
  value.capabilities = j;
  FROM_KEY(textDocumentSync);
  j["documentOnTypeFormattingProvider"]["firstTriggerCharacter"].get_to(
      value.formattingTrigger);
  j["completionProvider"]["resolveProvider"].get_to(value.resolveProvider);
  j["completionProvider"]["triggerCharacters"].get_to(value.completionTrigger);
  j["executeCommandProvider"]["commands"].get_to(value.executeCommands);
});

JSON_SERIALIZE(mergebot::lsp::ClangdCompileCommand,
               MAP_JSON(MAP_KEY(workingDirectory), MAP_KEY(compilationCommand)),
               {});

JSON_SERIALIZE(mergebot::lsp::ConfigurationSettings,
               MAP_JSON(MAP_KEY(compilationDatabaseChanges)), {});

JSON_SERIALIZE(mergebot::lsp::InitializationOptions,
               MAP_JSON(MAP_KEY(configSettings),
                        MAP_KEY(compilationDatabasePath),
                        MAP_KEY(fallbackFlags), MAP_KEY(clangdFileStatus)),
               {});

JSON_SERIALIZE(mergebot::lsp::ShowMessageParams, {}, {
  FROM_KEY(type);
  FROM_KEY(message)
});

JSON_SERIALIZE(mergebot::lsp::InitializeParams,
               MAP_JSON(MAP_KEY(processId), MAP_KEY(capabilities),
                        MAP_KEY(rootUri), MAP_KEY(initializationOptions),
                        MAP_KEY(rootPath)),
               {});

JSON_SERIALIZE(mergebot::lsp::Registration,
               MAP_JSON(MAP_KEY(id), MAP_KEY(method)), {})
JSON_SERIALIZE(mergebot::lsp::UnregistrationParams,
               MAP_JSON(MAP_KEY(unregisterations)), {})
JSON_SERIALIZE(mergebot::lsp::RegistrationParams,
               MAP_JSON(MAP_KEY(registrations)), {})
JSON_SERIALIZE(mergebot::lsp::DidOpenTextDocumentParams,
               MAP_JSON(MAP_KEY(textDocument)), {})
JSON_SERIALIZE(mergebot::lsp::DidCloseTextDocumentParams,
               MAP_JSON(MAP_KEY(textDocument)), {})
JSON_SERIALIZE(mergebot::lsp::TextDocumentContentChangeEvent,
               MAP_JSON(MAP_KEY(range), MAP_KEY(rangeLength), MAP_KEY(text)),
               {})
JSON_SERIALIZE(mergebot::lsp::DidChangeTextDocumentParams,
               MAP_JSON(MAP_KEY(textDocument), MAP_KEY(contentChanges),
                        MAP_KEY(wantDiagnostics)),
               {})
JSON_SERIALIZE(mergebot::lsp::FileEvent, MAP_JSON(MAP_KEY(uri), MAP_KEY(type)),
               {})
JSON_SERIALIZE(mergebot::lsp::DidChangeWatchedFilesParams,
               MAP_JSON(MAP_KEY(changes)), {})
JSON_SERIALIZE(mergebot::lsp::DocumentRangeFormattingParams,
               MAP_JSON(MAP_KEY(textDocument), MAP_KEY(range)), {})
JSON_SERIALIZE(mergebot::lsp::DidChangeConfigurationParams,
               MAP_JSON(MAP_KEY(settings)), {})
JSON_SERIALIZE(mergebot::lsp::DocumentOnTypeFormattingParams,
               MAP_JSON(MAP_KEY(textDocument), MAP_KEY(position), MAP_KEY(ch)),
               {})
JSON_SERIALIZE(mergebot::lsp::FoldingRangeParams,
               MAP_JSON(MAP_KEY(textDocument)), {})
JSON_SERIALIZE(mergebot::lsp::FoldingRange, {}, {
  FROM_KEY(startLine);
  FROM_KEY(startCharacter);
  FROM_KEY(endLine);
  FROM_KEY(endCharacter);
  FROM_KEY(kind);
})
JSON_SERIALIZE(mergebot::lsp::SelectionRange, {}, {
  FROM_KEY(range);
  if (j.contains("parent")) {
    value.parent = std::make_unique<mergebot::lsp::SelectionRange>();
    j.at("parent").get_to(*value.parent);
  }
})
JSON_SERIALIZE(mergebot::lsp::SelectionRangeParams,
               MAP_JSON(MAP_KEY(textDocument), MAP_KEY(positions)), {})
JSON_SERIALIZE(mergebot::lsp::DocumentFormattingParams,
               MAP_JSON(MAP_KEY(textDocument)), {})
JSON_SERIALIZE(mergebot::lsp::DocumentSymbolParams,
               MAP_JSON(MAP_KEY(textDocument)), {})
JSON_SERIALIZE(mergebot::lsp::DiagnosticRelatedInformation,
               MAP_JSON(MAP_KEY(location), MAP_KEY(message)), {
                 FROM_KEY(location);
                 FROM_KEY(message);
               })
JSON_SERIALIZE(mergebot::lsp::Diagnostic, {/*NOT REQUIRED*/}, {
  FROM_KEY(range);
  FROM_KEY(code);
  FROM_KEY(source);
  FROM_KEY(message);
  FROM_KEY(relatedInformation);
  FROM_KEY(category);
  FROM_KEY(codeActions);
})
JSON_SERIALIZE(mergebot::lsp::PublishDiagnosticsParams, {}, {
  FROM_KEY(uri);
  FROM_KEY(diagnostics);
})
JSON_SERIALIZE(mergebot::lsp::CodeActionContext, MAP_JSON(MAP_KEY(diagnostics)),
               {})
JSON_SERIALIZE(mergebot::lsp::CodeActionParams,
               MAP_JSON(MAP_KEY(textDocument), MAP_KEY(range),
                        MAP_KEY(context)),
               {})
JSON_SERIALIZE(mergebot::lsp::TweakArgs,
               MAP_JSON(MAP_KEY(file), MAP_KEY(selection), MAP_KEY(tweakID)), {
                 FROM_KEY(file);
                 FROM_KEY(selection);
                 FROM_KEY(tweakID);
               })
JSON_SERIALIZE(mergebot::lsp::WorkspaceEdit, MAP_JSON(MAP_KEY(changes)),
               { FROM_KEY(changes); })
JSON_SERIALIZE(mergebot::lsp::ExecuteCommandParams,
               MAP_JSON(MAP_KEY(command), MAP_KEY(workspaceEdit),
                        MAP_KEY(tweakArgs)),
               {})
JSON_SERIALIZE(mergebot::lsp::LspCommand,
               MAP_JSON(MAP_KEY(command), MAP_KEY(workspaceEdit),
                        MAP_KEY(tweakArgs), MAP_KEY(title)),
               {
                 FROM_KEY(command);
                 FROM_KEY(workspaceEdit);
                 FROM_KEY(tweakArgs);
                 FROM_KEY(title);
               })
JSON_SERIALIZE(mergebot::lsp::CodeAction,
               MAP_JSON(MAP_KEY(title), MAP_KEY(kind), MAP_KEY(diagnostics),
                        MAP_KEY(edit), MAP_KEY(command)),
               {
                 FROM_KEY(title);
                 FROM_KEY(kind);
                 FROM_KEY(diagnostics);
                 FROM_KEY(edit);
                 FROM_KEY(command)
               })
JSON_SERIALIZE(mergebot::lsp::SymbolInformation,
               MAP_JSON(MAP_KEY(name), MAP_KEY(kind), MAP_KEY(location),
                        MAP_KEY(containerName)),
               {
                 FROM_KEY(name);
                 FROM_KEY(kind);
                 FROM_KEY(location);
                 FROM_KEY(containerName)
               })

JSON_SERIALIZE(mergebot::lsp::SymbolDetails,
               MAP_JSON(MAP_KEY(name), MAP_KEY(containerName), MAP_KEY(usr),
                        MAP_KEY(id)),
               {FROM_KEY(name) FROM_KEY(containerName) FROM_KEY(usr)
                    FROM_KEY(id)})

JSON_SERIALIZE(mergebot::lsp::WorkspaceSymbolParams, MAP_JSON(MAP_KEY(query)),
               {})
JSON_SERIALIZE(mergebot::lsp::TextDocumentPositionParams,
               MAP_JSON(MAP_KEY(textDocument), MAP_KEY(position)), {})
JSON_SERIALIZE(mergebot::lsp::ApplyWorkspaceEditParams, MAP_JSON(MAP_KEY(edit)),
               {})
JSON_SERIALIZE(mergebot::lsp::CompletionContext,
               MAP_JSON(MAP_KEY(triggerKind), MAP_KEY(triggerCharacter)), {})
JSON_SERIALIZE(mergebot::lsp::CompletionParams,
               MAP_JSON(MAP_KEY(context), MAP_KEY(textDocument),
                        MAP_KEY(position)),
               {})
JSON_SERIALIZE(mergebot::lsp::MarkupContent, {}, {
  FROM_KEY(kind);
  FROM_KEY(value)
})
JSON_SERIALIZE(mergebot::lsp::Hover, {}, {
  FROM_KEY(contents);
  FROM_KEY(range)
})
JSON_SERIALIZE(mergebot::lsp::CompletionItem, {},
               {FROM_KEY(label) FROM_KEY(kind) FROM_KEY(detail)
                    FROM_KEY(documentation) FROM_KEY(sortText)
                        FROM_KEY(filterText) FROM_KEY(insertText)
                            FROM_KEY(insertTextFormat) FROM_KEY(textEdit)
                                FROM_KEY(additionalTextEdits) FROM_KEY(score)})
JSON_SERIALIZE(mergebot::lsp::ParameterInformation, {}, {
  FROM_KEY(labelString);
  FROM_KEY(labelOffsets);
  FROM_KEY(documentation);
})
JSON_SERIALIZE(mergebot::lsp::CompletionList, {}, {
  FROM_KEY(isIncomplete);
  FROM_KEY(items);
})
JSON_SERIALIZE(mergebot::lsp::SignatureInformation, {}, {
  FROM_KEY(label);
  FROM_KEY(documentation);
  FROM_KEY(parameters);
})
JSON_SERIALIZE(mergebot::lsp::TypeHierarchyParams,
               MAP_JSON(MAP_KEY(resolve), MAP_KEY(direction),
                        MAP_KEY(textDocument), MAP_KEY(position)),
               {})
JSON_SERIALIZE(mergebot::lsp::RenameParams,
               MAP_JSON(MAP_KEY(textDocument), MAP_KEY(position),
                        MAP_KEY(newName)),
               {})
JSON_SERIALIZE(mergebot::lsp::SignatureHelp, {}, {
  FROM_KEY(signatures);
  FROM_KEY(activeParameter);
  FROM_KEY(argListStart);
})
JSON_SERIALIZE(mergebot::lsp::ReferenceParams,
               MAP_JSON(MAP_KEY(textDocument), MAP_KEY(position)), {})
NLOHMANN_JSON_NAMESPACE_END

NLOHMANN_JSON_SERIALIZE_ENUM(
    mergebot::lsp::OffsetEncoding,
    {
        {mergebot::lsp::OffsetEncoding::UnsupportedEncoding, "unsupported"},
        {mergebot::lsp::OffsetEncoding::UTF8, "utf-8"},
        {mergebot::lsp::OffsetEncoding::UTF16, "utf-16"},
        {mergebot::lsp::OffsetEncoding::UTF32, "utf-32"},
    })
NLOHMANN_JSON_SERIALIZE_ENUM(
    mergebot::lsp::MarkupKind,
    {
        {mergebot::lsp::MarkupKind::PlainText, "plaintext"},
        {mergebot::lsp::MarkupKind::Markdown, "markdown"},
    })
NLOHMANN_JSON_SERIALIZE_ENUM(
    mergebot::lsp::ResourceOperationKind,
    {{mergebot::lsp::ResourceOperationKind::Create, "create"},
     {mergebot::lsp::ResourceOperationKind::Rename, "rename"},
     {mergebot::lsp::ResourceOperationKind::Delete, "delete"}})
NLOHMANN_JSON_SERIALIZE_ENUM(
    mergebot::lsp::FailureHandlingKind,
    {{mergebot::lsp::FailureHandlingKind::Abort, "abort"},
     {mergebot::lsp::FailureHandlingKind::Transactional, "transactional"},
     {mergebot::lsp::FailureHandlingKind::Undo, "undo"},
     {mergebot::lsp::FailureHandlingKind::TextOnlyTransactional,
      "textOnlyTransactional"}})

#endif  // MB_INCLUDE_MERGEBOT_LSP_PROTOCOL_H
