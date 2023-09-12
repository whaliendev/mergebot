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

class ClangdBasedTest : public ::testing::Test {
 protected:
  void SetUp() override {
    communicator = PipeCommunicator::create("./clangd", "clangd");
    ASSERT_TRUE(communicator) << "pipe to communicate with child process "
                                 "should construct successfully";
    rpcEndpoint = std::make_unique<JSONRpcEndpoint>(std::move(communicator));
    lspEndpoint = std::make_unique<LspEndpoint>(std::move(rpcEndpoint), 3);

    client = std::make_unique<LspClient>(std::move(lspEndpoint));
  }

  void TearDown() override {
    client->Shutdown();
    client->Exit();
  }

  std::unique_ptr<PipeCommunicator> communicator;
  std::unique_ptr<JSONRpcEndpoint> rpcEndpoint;
  std::unique_ptr<LspEndpoint> lspEndpoint;
  std::unique_ptr<LspClient> client;

  void initializeRocksdb() { auto returned = client->Initialize(rocksdbPath); }

 private:
  const std::string rocksdbPath = "/home/whalien/Desktop/rocksdb";
  const std::string avPath = "/home/whalien/Desktop/av";
};

TEST_F(ClangdBasedTest, LargeFile) {
  using namespace std::string_literals;
  client->Initialize(
      "/home/whalien/.mergebot/07dd6c3d8edc2e2df20f51c03c31b80d4ec25f1b/f9b2f0-5142b3/theirs"s);
  const std::string filePath =
      "/home/whalien/.mergebot/07dd6c3d8edc2e2df20f51c03c31b80d4ec25f1b/"
      "f9b2f0-5142b3/theirs/db/db_impl.cc";
  std::string fileContent = mergebot::util::file_get_content(filePath);
  client->DidOpen(filePath, fileContent);

  auto symbolOpt = client->SymbolInfo(filePath, Position{68, 15});
  ASSERT_TRUE(symbolOpt.has_value());
  spdlog::info("details: {}", symbolOpt.value().dump(2));

  SymbolDetails details = symbolOpt.value();
}

TEST_F(ClangdBasedTest, Commnucation) {
  this->initializeRocksdb();

  const std::string filePath =
      "/home/whalien/Desktop/rocksdb/db/transaction_log_impl.h";
  URIForFile file(filePath);
  std::string fileContent = mergebot::util::file_get_content(filePath);
  client->DidOpen(file, fileContent);

  std::optional<json> symbolDetails =
      client->SymbolInfo(file, Position{77, 28});
  std::optional<json> references = client->References(file, Position{77, 8});
  std::optional<json> declaration =
      client->GoToDeclaration(file, Position{77, 28});
  std::optional<json> definition =
      client->GoToDefinition(file, Position{77, 28});
  std::optional<json> completion = client->Completion(file, Position{77, 28});
  std::optional<json> signature = client->SignatureHelp(file, Position{77, 28});

  if (references.has_value()) {
    spdlog::info("debug output: {}", references.value().dump(2));
  }

  spdlog::info(
      "symbolDetails: {}, references: {}, declaration: {}, definition: {}, "
      "completion: {}, signature: {}",
      symbolDetails.value().size(), references.value().size(),
      declaration.value().size(), definition.value().size(),
      completion.value().size(), signature.value().size());
}

TEST_F(ClangdBasedTest, EmptyMacro) {
  std::string workspaceRoot =
      "/home/whalien/.conan2/p/rapid4cabb31a09329/p/include/rapidjson";
  client->Initialize(workspaceRoot);
  const std::string filePath =
      "/home/whalien/.conan2/p/rapid4cabb31a09329/p/include/rapidjson/"
      "document.h";
  URIForFile file(filePath);
  std::string fileContent = mergebot::util::file_get_content(filePath);

  client->DidOpen(file, fileContent);
  std::optional<json> symbolDetails =
      client->SymbolInfo(file, Position{141, 19});
  std::optional<json> references = client->References(file, Position{141, 19});
  std::optional<json> declaration =
      client->GoToDeclaration(file, Position{141, 19});
  std::optional<json> definition =
      client->GoToDefinition(file, Position{141, 19});
  std::optional<json> completion = client->Completion(file, Position{141, 19});
  std::optional<json> signature =
      client->SignatureHelp(file, Position{141, 19});

  if (signature.has_value()) {
    spdlog::info("debug output: {}", signature.value().dump(2));
  }

  spdlog::info(
      "symbolDetails: {}, references: {}, declaration: {}, definition: {}, "
      "completion: {}, signature: {}",
      symbolDetails.value().size(), references.value().size(),
      declaration.value().size(), definition.value().size(),
      completion.value().size(), signature.value().size());
}

TEST_F(ClangdBasedTest, SwitchSourceHeader) {
  this->initializeRocksdb();
  const std::string filePath =
      "/home/whalien/Desktop/rocksdb/db/transaction_log_impl.h";
  URIForFile file(filePath);
  std::string fileContent = mergebot::util::file_get_content(filePath);
  client->DidOpen(file, fileContent);
  //  auto sourceOrHeaderOpt = client->SwitchSourceHeader(file);
  //  ASSERT_TRUE(sourceOrHeaderOpt.has_value());
  //  URIForFile sourceOrHeader = sourceOrHeaderOpt.value();
  //  std::string sourceOrHeaderContent =
  //      mergebot::util::file_get_content(sourceOrHeader.path());
  //  client->DidOpen(sourceOrHeader, sourceOrHeaderContent);

  auto references = client->References(file, Position{60, 14});
  ASSERT_TRUE(references.has_value());
  std::vector<ReferenceLocation> rlocation = references.value();
  std::vector<std::string> expectedContainerNames = {
      "rocksdb::LogFileImpl::LogFileImpl", "rocksdb::LogFileImpl::PathName",
      "rocksdb::LogFileImpl::Type"};
  ASSERT_THAT(rlocation, ::testing::SizeIs(3));
  for (size_t i = 0; i < rlocation.size(); ++i) {
    ASSERT_EQ(expectedContainerNames[i], rlocation[i].containerName.value());
  }
}

TEST_F(ClangdBasedTest, SymbolDetails) {
  this->initializeRocksdb();
  const std::string filePath =
      "/home/whalien/Desktop/rocksdb/db/transaction_log_impl.h";
  URIForFile file(filePath);
  std::string fileContent = mergebot::util::file_get_content(filePath);
  client->DidOpen(file, fileContent);
  auto symbolOpt = client->SymbolInfo(file, Position{16, 10});
  ASSERT_TRUE(symbolOpt.has_value());
  spdlog::info("details: {}", symbolOpt.value().dump(2));

  SymbolDetails details = symbolOpt.value();
}

TEST_F(ClangdBasedTest, Function) {
  this->initializeRocksdb();
  const std::string filePath = "/home/whalien/Desktop/rocksdb/util/bloom.cc";
  URIForFile file(filePath);
  std::string fileContent = mergebot::util::file_get_content(filePath);
  client->DidOpen(file, fileContent);
  auto symbolOpt = client->SymbolInfo(file, Position{16, 0});
  ASSERT_TRUE(symbolOpt.has_value());
  spdlog::info("details: {}", symbolOpt.value().dump(2));
  SymbolDetails details = symbolOpt.value();

  symbolOpt = client->SymbolInfo(file, Position{17, 16});
  ASSERT_TRUE(symbolOpt.has_value());
  spdlog::info("details: {}", symbolOpt.value().dump(2));

  symbolOpt = client->SymbolInfo(file, Position{35, 11});
  ASSERT_TRUE(symbolOpt.has_value());
  spdlog::info("details: {}", symbolOpt.value().dump(2));

  symbolOpt = client->SymbolInfo(file, Position{50, 15});
  ASSERT_TRUE(symbolOpt.has_value());
  spdlog::info("details: {}", symbolOpt.value().dump(2));
}

TEST_F(ClangdBasedTest, FuncDef) {
  this->initializeRocksdb();
  const std::string filePath =
      "/home/whalien/Desktop/rocksdb/db/column_family.cc";
  URIForFile file(filePath);
  std::string fileContent = mergebot::util::file_get_content(filePath);
  client->DidOpen(file, fileContent);

  URIForFile header("/home/whalien/Desktop/rocksdb/db/column_family.h");
  std::string headerContent = mergebot::util::file_get_content(header.path());
  client->DidOpen(header, headerContent);

  auto symbolOpt = client->SymbolInfo(file, Position{25, 24});
  ASSERT_TRUE(symbolOpt.has_value());
  spdlog::info("details: {}", symbolOpt.value().dump(2));
  SymbolDetails details = symbolOpt.value();

  symbolOpt = client->SymbolInfo(file, Position{33, 24});
  ASSERT_TRUE(symbolOpt.has_value());
  spdlog::info("details: {}", symbolOpt.value().dump(2));

  symbolOpt = client->SymbolInfo(file, Position{46, 33});
  ASSERT_TRUE(symbolOpt.has_value());
  spdlog::info("details: {}", symbolOpt.value().dump(2));

  symbolOpt = client->SymbolInfo(file, Position{51, 12});
  ASSERT_TRUE(symbolOpt.has_value());
  spdlog::info("details: {}", symbolOpt.value().dump(2));

  symbolOpt = client->SymbolInfo(file, Position{57, 20});
  ASSERT_TRUE(symbolOpt.has_value());
  spdlog::info("details: {}", symbolOpt.value().dump(2));
}
