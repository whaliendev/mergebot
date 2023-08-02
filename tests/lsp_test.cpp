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
      std::make_unique<LspEndpoint>(std::move(rpcEndpoint), 20);

  LspClient client(std::move(lspEndpoint));

  std::string workspaceRoot =
      "/home/whalien/Desktop/mergebot/aosp/frameworks/av";
  const std::string filePath =
      "/home/whalien/Desktop/mergebot/aosp/frameworks/av/media/libmedia/"
      "IStreamSource.cpp";
  URIForFile file(filePath);
  std::string fileContent = mergebot::util::file_get_content(filePath);

  auto returned = client.Initialize(workspaceRoot);
  client.DidOpen(file, fileContent);
  std::optional<json> symbolDetails = client.SymbolInfo(file, Position{82, 25});
  std::optional<json> references = client.References(file, Position{82, 25});
  std::optional<json> declaration =
      client.GoToDeclaration(file, Position{82, 25});
  std::optional<json> definition =
      client.GoToDefinition(file, Position{82, 25});
  std::optional<json> completion = client.Completion(file, Position{82, 25});
  std::optional<json> signature = client.SignatureHelp(file, Position{82, 25});

  client.Shutdown();
  client.Exit();

  spdlog::info(
      "symbolDetails: {}, references: {}, declaration: {}, definition: {}, "
      "completion: {}, signature: {}",
      symbolDetails.value().size(), references.value().size(),
      declaration.value().size(), definition.value().size(),
      completion.value().size(), signature.value().size());
}