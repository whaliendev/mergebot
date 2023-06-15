//
// Created by whalien on 11/06/23.
//

#ifndef MB_INCLUDE_MERGEBOT_LSP_CLIENT_H
#define MB_INCLUDE_MERGEBOT_LSP_CLIENT_H

#include <spdlog/spdlog.h>
#include <sys/wait.h>

#include <nlohmann/json.hpp>
#include <shared_mutex>

#include "mergebot/utils/noncopyable.h"
#include "protocol.h"
#include "transport.h"

namespace mergebot {
namespace lsp {
using json = nlohmann::json;
using value = json;

class JSONRpcEndpoint final {
 public:
  using RpcResponseBody = json;
  using RpcRequestBody = json;
  JSONRpcEndpoint(int infd, int outfd) : stdin(infd), stdout(outfd) {}
  ssize_t SendRequest(const RpcRequestBody &json);
  std::optional<RpcResponseBody> RecvResponse();

 private:
  std::string fillMessageHeader(const std::string &json);
  std::string readLine();
  std::string readMessageContent(size_t len);

  const char *LEN_HEADER = "Content-Length: ";
  const char *TYPE_HEADER = "Content-Type: ";

  int stdin = -1, stdout = -1;
  std::shared_mutex rwMutex;
};

class LspEndpoint final {
 public:
  using JSONRpcResult = json;
  using JSONRpcError = json;
  using JSONRpcParams = json;

  explicit LspEndpoint(
      std::unique_ptr<JSONRpcEndpoint> rpcEndpoint, int timeout = 3,
      const std::unordered_map<std::string, std::function<void(const json &)>>
          methodCallbacks = {},
      const std::unordered_map<std::string, std::function<void(const json &)>>
          notifyCallbacks = {})
      : rpcEndpoint(std::move(rpcEndpoint)),
        methodCallbacks(methodCallbacks),
        notifyCallbacks(notifyCallbacks),
        timeout(timeout),
        shutdownFlag(false) {}

  void SendNotification(std::string_view method,
                        const JSONRpcParams &params = {});

  void SendResponse(int rpcId, const json &result, const json &error);

  std::optional<JSONRpcResult> CallMethod(std::string_view method,
                                          const json &params = {});

  void Stop();

  void operator()();

 private:
  void handleResult(int rpcId, const JSONRpcResult &result,
                    const JSONRpcError &error);
  void sendMessage(std::string_view method, const json &params, int id = -1);

  std::unique_ptr<JSONRpcEndpoint> rpcEndpoint;
  std::unordered_map<std::string, std::function<void(const json &)>>
      methodCallbacks;
  std::unordered_map<std::string, std::function<void(const json &)>>
      notifyCallbacks;
  std::mutex cvMutex;
  std::unordered_map<int, std::shared_ptr<std::condition_variable>> eventDict;
  std::unordered_map<int, std::pair<JSONRpcResult, JSONRpcError>> responseDict;

  static int ID;
  const char *jsonrpc = "2.0";
  int timeout = 3;
  bool shutdownFlag = false;
};

class LspClient final {
 public:
  using JSONRpcResult = LspEndpoint::JSONRpcResult;
  explicit LspClient(std::unique_ptr<LspEndpoint> lspEndpoint)
      : lspEndpoint(std::move(lspEndpoint)) {}

  ~LspClient() {
    if (endpointThread.joinable()) {
      endpointThread.join();
    }
  }

  std::optional<JSONRpcResult> Initialize(URIForFile rootUri) {
    std::reference_wrapper<LspEndpoint> ref(*lspEndpoint);
    endpointThread = std::thread(ref);

    InitializeParams params;
    params.processId = getpid();
    params.rootUri = rootUri;
    return lspEndpoint->CallMethod("initialize", params);
  }

  void Initialized() { lspEndpoint->SendNotification("initialized"); }

  std::optional<JSONRpcResult> Shutdown() {
    lspEndpoint->Stop();
    return lspEndpoint->CallMethod("shutdown");
  }

  void Exit() { lspEndpoint->SendNotification("exit"); }

  std::optional<JSONRpcResult> Sync() {
    return lspEndpoint->CallMethod("sync");
  }

  std::optional<JSONRpcResult> RegisterCapability() {
    // TODO(hwa): extend
    return lspEndpoint->CallMethod("client/registerCapability");
  }

  void DidOpen(URIForFile uri, std::string_view text,
               std::string_view languageId = "cpp") {
    DidOpenTextDocumentParams params;
    params.textDocument.uri = std::move(uri);
    params.textDocument.languageId = languageId;
    params.textDocument.text = text;
    lspEndpoint->SendNotification("textDocument/didOpen", params);
  }

  void DidChange(URIForFile uri,
                 std::vector<TextDocumentContentChangeEvent> &changes,
                 bool wantDiagnostics = true) {
    DidChangeTextDocumentParams params;
    params.textDocument.uri = std::move(uri);
    params.contentChanges = std::move(changes);
    params.wantDiagnostics = wantDiagnostics;
    lspEndpoint->SendNotification("textDocument/didChange", params);
  }

  void DidClose(URIForFile uri) {
    DidCloseTextDocumentParams params;
    params.textDocument.uri = std::move(uri);
    lspEndpoint->SendNotification("textDocument/didClose", params);
  }

  std::optional<JSONRpcResult> RangeFormatting(URIForFile uri, Range range) {
    DocumentRangeFormattingParams params;
    params.textDocument.uri = std::move(uri);
    params.range = range;
    return lspEndpoint->CallMethod("textDocument/rangeFormatting", params);
  }

  std::optional<JSONRpcResult> FoldingRange(URIForFile uri) {
    FoldingRangeParams params;
    params.textDocument.uri = std::move(uri);
    return lspEndpoint->CallMethod("textDocument/foldingRange", params);
  }

  std::optional<JSONRpcResult> SelectionRange(
      URIForFile uri, std::vector<Position> &positions) {
    SelectionRangeParams params;
    params.textDocument.uri = std::move(uri);
    params.positions = std::move(positions);
    return lspEndpoint->CallMethod("textDocument/selectionRange", params);
  }

  std::optional<JSONRpcResult> OnTypeFormatting(URIForFile uri,
                                                Position position,
                                                std::string_view ch) {
    DocumentOnTypeFormattingParams params;
    params.textDocument.uri = std::move(uri);
    params.position = position;
    params.ch = ch;
    return lspEndpoint->CallMethod("textDocument/onTypeFormatting",
                                   std::move(params));
  }

  std::optional<JSONRpcResult> Formatting(URIForFile uri) {
    DocumentFormattingParams params;
    params.textDocument.uri = std::move(uri);
    return lspEndpoint->CallMethod("textDocument/formatting",
                                   std::move(params));
  }

  std::optional<JSONRpcResult> CodeAction(URIForFile uri, Range range,
                                          CodeActionContext context) {
    CodeActionParams params;
    params.textDocument.uri = std::move(uri);
    params.range = range;
    params.context = std::move(context);
    return lspEndpoint->CallMethod("textDocument/codeAction",
                                   std::move(params));
  }

  std::optional<JSONRpcResult> Completion(URIForFile uri, Position position,
                                          CompletionContext context = {}) {
    CompletionParams params;
    params.textDocument.uri = std::move(uri);
    params.position = position;
    params.context = context;
    return lspEndpoint->CallMethod("textDocument/completion", params);
  }

  std::optional<JSONRpcResult> SignatureHelp(URIForFile uri,
                                             Position position) {
    TextDocumentPositionParams params;
    params.textDocument.uri = std::move(uri);
    params.position = position;
    return lspEndpoint->CallMethod("textDocument/signatureHelp",
                                   std::move(params));
  }

  std::optional<JSONRpcResult> GoToDefinition(URIForFile uri,
                                              Position position) {
    TextDocumentPositionParams params;
    params.textDocument.uri = std::move(uri);
    params.position = position;
    return lspEndpoint->CallMethod("textDocument/definition",
                                   std::move(params));
  }

  std::optional<JSONRpcResult> GoToDeclaration(URIForFile uri,
                                               Position position) {
    TextDocumentPositionParams params;
    params.textDocument.uri = std::move(uri);
    params.position = position;
    return lspEndpoint->CallMethod("textDocument/declaration",
                                   std::move(params));
  }

  std::optional<JSONRpcResult> References(URIForFile uri, Position position) {
    ReferenceParams params;
    params.textDocument.uri = std::move(uri);
    params.position = position;
    return lspEndpoint->CallMethod("textDocument/references",
                                   std::move(params));
  }

  std::optional<JSONRpcResult> SwitchSourceHeader(URIForFile uri) {
    TextDocumentIdentifier params;
    params.uri = std::move(uri);
    return lspEndpoint->CallMethod("textDocument/references",
                                   std::move(params));
  }

  std::optional<JSONRpcResult> Rename(URIForFile uri, Position position,
                                      std::string_view newName) {
    RenameParams params;
    params.textDocument.uri = std::move(uri);
    params.position = position;
    params.newName = newName;
    return lspEndpoint->CallMethod("textDocument/rename", std::move(params));
  }

  std::optional<JSONRpcResult> Hover(URIForFile uri, Position position) {
    TextDocumentPositionParams params;
    params.textDocument.uri = std::move(uri);
    params.position = position;
    return lspEndpoint->CallMethod("textDocument/hover", std::move(params));
  }

  std::optional<JSONRpcResult> DocumentSymbol(URIForFile uri) {
    DocumentSymbolParams params;
    params.textDocument.uri = std::move(uri);
    return lspEndpoint->CallMethod("textDocument/documentSymbol",
                                   std::move(params));
  }

  std::optional<JSONRpcResult> DocumentColor(URIForFile uri) {
    DocumentSymbolParams params;
    params.textDocument.uri = std::move(uri);
    return lspEndpoint->CallMethod("textDocument/documentColor",
                                   std::move(params));
  }

  std::optional<JSONRpcResult> DocumentHighlight(URIForFile uri,
                                                 Position position) {
    TextDocumentPositionParams params;
    params.textDocument.uri = std::move(uri);
    params.position = position;
    return lspEndpoint->CallMethod("textDocument/documentHighlight",
                                   std::move(params));
  }

  std::optional<JSONRpcResult> SymbolInfo(URIForFile uri, Position position) {
    TextDocumentPositionParams params;
    params.textDocument.uri = std::move(uri);
    params.position = position;
    return lspEndpoint->CallMethod("textDocument/symbolInfo",
                                   std::move(params));
  }

  std::optional<JSONRpcResult> TypeHierarchy(URIForFile uri, Position position,
                                             TypeHierarchyDirection direction,
                                             int resolve) {
    TypeHierarchyParams params;
    params.textDocument.uri = std::move(uri);
    params.position = position;
    params.direction = direction;
    params.resolve = resolve;
    return lspEndpoint->CallMethod("textDocument/typeHierarchy",
                                   std::move(params));
  }

  std::optional<JSONRpcResult> WorkspaceSymbol(std::string_view query) {
    WorkspaceSymbolParams params;
    params.query = query;
    return lspEndpoint->CallMethod("workspace/symbol", std::move(params));
  }

  std::optional<JSONRpcResult> ExecuteCommand(
      std::string_view cmd, TweakArgs tweakArgs = {},
      WorkspaceEdit workspaceEdit = {}) {
    ExecuteCommandParams params;
    params.tweakArgs = tweakArgs;
    params.workspaceEdit = workspaceEdit;
    params.command = cmd;
    return lspEndpoint->CallMethod("workspace/executeCommand",
                                   std::move(params));
  }

  std::optional<JSONRpcResult> DidChangeWatchedFiles(
      std::vector<FileEvent> &changes) {
    DidChangeWatchedFilesParams params;
    params.changes = std::move(changes);
    return lspEndpoint->CallMethod("workspace/didChangeWatchedFiles",
                                   std::move(params));
  }

  std::optional<JSONRpcResult> DidChangeConfiguration(
      ConfigurationSettings &settings) {
    DidChangeConfigurationParams params;
    params.settings = std::move(settings);
    return lspEndpoint->CallMethod("workspace/didChangeConfiguration",
                                   std::move(params));
  }

 private:
  std::unique_ptr<LspEndpoint> lspEndpoint;
  std::thread endpointThread;
};
}  // namespace lsp
}  // namespace mergebot

#endif  // MB_INCLUDE_MERGEBOT_LSP_CLIENT_H
