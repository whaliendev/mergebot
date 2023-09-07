//
// Created by whalien on 14/06/23.
//
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <nlohmann/json.hpp>  // for std::vector serialization and deserialization

#include "mergebot/lsp/client.h"
#include "mergebot/lsp/communicator.h"
#include "mergebot/lsp/protocol.h"
#include "mergebot/utils/fileio.h"

using namespace mergebot::lsp;
TEST(LSP, Commnucation) {
  auto pipeCommunicator = PipeCommunicator::create("./clangd", "clangd");
  EXPECT_TRUE(pipeCommunicator) << "pipe to communicate with child process "
                                   "should construct successfully";
  std::unique_ptr<JSONRpcEndpoint> rpcEndpoint =
      std::make_unique<JSONRpcEndpoint>(std::move(pipeCommunicator));
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
      completion.value().size(), signature.value().size());
}

TEST(LSP, EmptyMacro) {
  auto pipeCommunicator = PipeCommunicator::create("./clangd", "clangd");
  EXPECT_TRUE(pipeCommunicator) << "pipe to communicate with child process "
                                   "should construct successfully";
  std::unique_ptr<JSONRpcEndpoint> rpcEndpoint =
      std::make_unique<JSONRpcEndpoint>(std::move(pipeCommunicator));
  std::unique_ptr<LspEndpoint> lspEndpoint =
      std::make_unique<LspEndpoint>(std::move(rpcEndpoint), 3);

  LspClient client(std::move(lspEndpoint));

  std::string workspaceRoot =
      "/home/whalien/.conan2/p/rapid4cabb31a09329/p/include/rapidjson";
  auto returned = client.Initialize(workspaceRoot);
  const std::string filePath =
      "/home/whalien/.conan2/p/rapid4cabb31a09329/p/include/rapidjson/"
      "document.h";
  URIForFile file(filePath);
  std::string fileContent = mergebot::util::file_get_content(filePath);

  client.DidOpen(file, fileContent);
  std::optional<json> symbolDetails =
      client.SymbolInfo(file, Position{141, 19});
  std::optional<json> references = client.References(file, Position{141, 19});
  std::optional<json> declaration =
      client.GoToDeclaration(file, Position{141, 19});
  std::optional<json> definition =
      client.GoToDefinition(file, Position{141, 19});
  std::optional<json> completion = client.Completion(file, Position{141, 19});
  std::optional<json> signature = client.SignatureHelp(file, Position{141, 19});

  if (signature.has_value()) {
    spdlog::info("debug output: {}", signature.value().dump(2));
  }
  client.Shutdown();
  client.Exit();

  spdlog::info(
      "symbolDetails: {}, references: {}, declaration: {}, definition: {}, "
      "completion: {}, signature: {}",
      symbolDetails.value().size(), references.value().size(),
      declaration.value().size(), definition.value().size(),
      completion.value().size(), signature.value().size());
}

TEST(Lsp, SwitchSourceHeader) {
  auto communicator = PipeCommunicator::create("./clangd", "clangd");
  EXPECT_TRUE(communicator) << "pipe to communicate with child process "
                               "should construct successfully";
  std::unique_ptr<JSONRpcEndpoint> rpcEndpoint =
      std::make_unique<JSONRpcEndpoint>(std::move(communicator));
  std::unique_ptr<LspEndpoint> lspEndpoint =
      std::make_unique<LspEndpoint>(std::move(rpcEndpoint), 3);

  LspClient client(std::move(lspEndpoint));

  const std::string workspaceRoot = "/home/whalien/Desktop/rocksdb";
  auto returned = client.Initialize(workspaceRoot);
  const std::string filePath =
      "/home/whalien/Desktop/rocksdb/db/transaction_log_impl.h";
  URIForFile file(filePath);
  std::string fileContent = mergebot::util::file_get_content(filePath);
  client.DidOpen(file, fileContent);
  //  auto sourceOrHeaderOpt = client.SwitchSourceHeader(file);
  //  ASSERT_TRUE(sourceOrHeaderOpt.has_value());
  //  URIForFile sourceOrHeader = sourceOrHeaderOpt.value();
  //  std::string sourceOrHeaderContent =
  //      mergebot::util::file_get_content(sourceOrHeader.path());
  //  client.DidOpen(sourceOrHeader, sourceOrHeaderContent);

  auto references = client.References(file, Position{60, 14});
  ASSERT_TRUE(references.has_value());
  std::vector<ReferenceLocation> rlocation = references.value();
  std::vector<std::string> expectedContainerNames = {
      "rocksdb::LogFileImpl::LogFileImpl", "rocksdb::LogFileImpl::PathName",
      "rocksdb::LogFileImpl::Type"};
  ASSERT_THAT(rlocation, ::testing::SizeIs(3));
  for (size_t i = 0; i < rlocation.size(); ++i) {
    ASSERT_EQ(expectedContainerNames[i], rlocation[i].containerName.value());
  }

  client.Shutdown();
  client.Exit();
}

TEST(Lsp, SymbolDetails) {
  auto communicator = PipeCommunicator::create("./clangd", "clangd");
  EXPECT_TRUE(communicator) << "pipe to communicate with child process "
                               "should construct successfully";
  std::unique_ptr<JSONRpcEndpoint> rpcEndpoint =
      std::make_unique<JSONRpcEndpoint>(std::move(communicator));
  std::unique_ptr<LspEndpoint> lspEndpoint =
      std::make_unique<LspEndpoint>(std::move(rpcEndpoint), 3);

  LspClient client(std::move(lspEndpoint));

  const std::string workspaceRoot = "/home/whalien/Desktop/rocksdb";
  auto returned = client.Initialize(workspaceRoot);
  const std::string filePath =
      "/home/whalien/Desktop/rocksdb/db/transaction_log_impl.h";
  URIForFile file(filePath);
  std::string fileContent = mergebot::util::file_get_content(filePath);
  client.DidOpen(file, fileContent);
  auto symbolOpt = client.SymbolInfo(file, Position{16, 10});
  ASSERT_TRUE(symbolOpt.has_value());
  spdlog::info("details: {}", symbolOpt.value().dump(2));

  SymbolDetails details = symbolOpt.value();

  client.Shutdown();
  client.Exit();
}
