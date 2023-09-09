//
// Created by whalien on 11/06/23.
//

#ifndef MB_INCLUDE_MERGEBOT_LSP_CLIENT_H
#define MB_INCLUDE_MERGEBOT_LSP_CLIENT_H

#include <spdlog/spdlog.h>
#include <sys/wait.h>

#include <nlohmann/json.hpp>
#include <shared_mutex>

#include "communicator.h"
#include "mergebot/filesystem.h"
#include "mergebot/utils/noncopyable.h"
#include "protocol.h"

namespace mergebot {
namespace lsp {
using json = nlohmann::json;
using value = json;

/**
 * @class JSONRpcEndpoint
 * @brief Handles the low-level communication details for JSON RPC.
 *
 * This class is responsible for sending and receiving raw messages over the
 * network. It uses a Communicator object to abstract the details of the
 * communication method (e.g., pipe, socket).
 */
class JSONRpcEndpoint final {
 public:
  using RpcResponseBody = json;
  using RpcRequestBody = json;
  explicit JSONRpcEndpoint(std::unique_ptr<Communicator> communicator)
      : communicator{std::move(communicator)} {}

  /**
   * @brief Sends a JSON RPC request over the network.
   *
   * This method serializes the request to a JSON string, adds the necessary
   * headers, and sends it over the network using the Communicator.
   */
  ssize_t SendRequest(const RpcRequestBody &json);

  /**
   * @brief Receives a JSON RPC response from the network.
   *
   * This method reads the headers and content of a response from the network,
   * parses the content into a JSON object, and returns it.
   */
  std::optional<RpcResponseBody> RecvResponse();

 private:
  std::string fillMessageHeader(const std::string &json);
  std::string readLine();
  std::string readMessageContent(size_t len);

  const char *LEN_HEADER = "Content-Length: ";
  const char *TYPE_HEADER = "Content-Type: ";

  std::unique_ptr<Communicator> communicator;
  std::shared_mutex rwMutex;
};

/**
 * @class LspEndpoint
 * @brief Provides a high-level interface for communicating with a Language
 * Server.
 *
 * This class uses a JSONRpcEndpoint to send and receive messages. It provides
 * methods for sending notifications and calling methods on the server. It also
 * handles the details of matching responses to requests.
 */
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

  /**
   * @brief Sends a notification to the Language Server.
   *
   * This method sends a notification to the server. Notifications are requests
   * that do not require a response.
   */
  void SendNotification(std::string_view method,
                        const JSONRpcParams &params = {});

  /**
   * @brief Sends a response to the Language Server.
   *
   * This method is used internally to send a response back to the server after
   * a request has been processed. It is called by the handleResult method after
   * the result of a request has been computed.
   *
   * @param id The ID of the request to which this is a response.
   * @param result The result of the request, to be included in the response.
   * @param error Any error that occurred while processing the request, to be
   * included in the response.
   */
  void SendResponse(int rpcId, const json &result, const json &error);

  /**
   * @brief Calls a method on the Language Server and waits for the response.
   *
   * This method sends a request to the server, waits for the response, and
   * returns the result. If an error occurs, it logs the error and returns
   * nullopt.
   */
  std::optional<JSONRpcResult> CallMethod(std::string_view method,
                                          const json &params = {});

  /**
   * @brief Stops the LspEndpoint.
   *
   * This method sets a flag that causes the operator() method to return. It
   * also wakes up any threads that are waiting for a response.
   */
  void Stop();

  /**
   * @brief The main loop of the LspEndpoint.
   *
   * This method continuously receives responses from the server and handles
   * them. It matches responses to requests and notifies the waiting threads. It
   * also handles notifications from the server.
   */
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
  /// default ctor
  LspClient() : lspEndpoint{nullptr}, endpointThread{} {};

  /// parameterized ctor
  explicit LspClient(std::unique_ptr<LspEndpoint> lspEndpoint)
      : lspEndpoint(std::move(lspEndpoint)) {
    std::reference_wrapper<LspEndpoint> ref(*this->lspEndpoint);
    endpointThread = std::thread(ref);
  }

  // deleted copy constructor and copy assignment operator for doc use
  LspClient(const LspClient &) = delete;
  LspClient &operator=(const LspClient &) = delete;

  // move constructor
  LspClient(LspClient &&other) noexcept
      : lspEndpoint(std::move(other.lspEndpoint)),
        endpointThread(std::move(other.endpointThread)) {
    other.endpointThread = std::thread();  // Reset the other's thread
  }

  // move assignment operator
  LspClient &operator=(LspClient &&other) noexcept {
    if (this != &other) {
      this->swap(other);
    }
    return *this;
  }

  // member swap function
  void swap(LspClient &other) noexcept {
    using std::swap;
    swap(lspEndpoint, other.lspEndpoint);
    swap(endpointThread, other.endpointThread);
  }

  // Friend swap function for std::swap overload
  friend void swap(LspClient &lhs, LspClient &rhs) noexcept { lhs.swap(rhs); }

  // dtor
  ~LspClient() {
    if (endpointThread.joinable()) {
      endpointThread.join();
    }
  }

  std::optional<JSONRpcResult> Initialize(URIForFile rootUri) {
    assert(lspEndpoint != nullptr && "lspEndpoint should not be nullptr");
    assert(endpointThread.joinable() && "endpointThread should be joinable");

    InitializeParams params;
    params.processId = getpid();
    params.rootUri = rootUri;
    //  https://clangd.llvm.org/extensions#compilation-commands
    params.initializationOptions.compilationDatabasePath =
        (fs::path(rootUri.path()) / "build").string();
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
    params.textDocument.version = 1;
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
    return lspEndpoint->CallMethod("textDocument/switchSourceHeader",
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
