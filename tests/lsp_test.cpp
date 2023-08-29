//
// Created by whalien on 14/06/23.
//
#include <gtest/gtest.h>

#include "mergebot/lsp/client.h"
#include "mergebot/utils/fileio.h"

using namespace mergebot::lsp;
TEST(LspTest, Commnucation) {
  std::unique_ptr<JSONRpcEndpoint> rpcEndpoint =
      std::make_unique<JSONRpcEndpoint>("/usr/local/bin/clangd", "clangd");
  std::unique_ptr<LspEndpoint> lspEndpoint =
      std::make_unique<LspEndpoint>(std::move(rpcEndpoint), 3);

  LspClient client(std::move(lspEndpoint));

  std::string workspaceRoot = "/home/whalien/Desktop/rocksdb";
  const std::string filePath =
      "/home/whalien/Desktop/rocksdb/db/transaction_log_impl.h";
  URIForFile file(filePath);
  std::string fileContent = mergebot::util::file_get_content(filePath);

  auto returned = client.Initialize(workspaceRoot);
  client.DidOpen(file, fileContent);
  std::optional<json> symbolDetails = client.SymbolInfo(file, Position{77, 28});
  std::optional<json> references = client.References(file, Position{77, 8});
  std::optional<json> declaration =
      client.GoToDeclaration(file, Position{77, 28});
  std::optional<json> definition =
      client.GoToDefinition(file, Position{77, 28});
  std::optional<json> completion = client.Completion(file, Position{77, 28});
  std::optional<json> signature = client.SignatureHelp(file, Position{77, 28});

  if (references.has_value()) {
    spdlog::info("debug output: {}", references.value().dump(2));
  }
  client.Shutdown();
  client.Exit();

  spdlog::info(
      "symbolDetails: {}, references: {}, declaration: {}, definition: {}, "
      "completion: {}, signature: {}",
      symbolDetails.value().size(), references.value().size(),
      declaration.value().size(), definition.value().size(),
      /*completion.value().size(),*/ signature.value().size());
}
