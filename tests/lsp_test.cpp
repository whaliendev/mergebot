//
// Created by whalien on 14/06/23.
//
#include <gtest/gtest.h>

#include "mergebot/lsp/client.h"
#include "mergebot/utils/fileio.h"

using namespace mergebot::lsp;
TEST(LspTest, Commnucation) {
  int pipeIn[2], pipeOut[2];
  const char* executable = "clangd";
  const char* args = "";

  if (pipe(pipeIn) == -1) {
    spdlog::error(
        "failed to create pipe to communicate with {}, err message: {}",
        executable, strerror(errno));
    //    goto handle;
  }

  if (pipe(pipeOut) == -1) {
    spdlog::error(
        "failed to create pipe to communicate with {}, err message: {}",
        executable, strerror(errno));
    //    goto handle;
  }

  int readIn = pipeIn[0];
  int writeIn = pipeIn[1];
  int readOut = pipeOut[0];
  int writeOut = pipeOut[1];

  pid_t processId = fork();
  if (processId == -1) {
    spdlog::error("fork {} failed, err message: {}", executable,
                  strerror(errno));
    //    goto handle;
  } else if (processId == 0) {
    // child process
    close(writeIn);
    close(readOut);
    dup2(readIn, STDIN_FILENO);
    dup2(writeOut, STDOUT_FILENO);
    close(readIn);
    close(writeOut);
    execl("/usr/local/bin/clangd", "clangd", NULL);

    // execl returned, an error occurred
    spdlog::error("failed to fork {}, error message: {}", executable,
                  strerror(errno));
  } else {
    // parent process
    close(readIn);
    close(writeOut);
  }

  std::unique_ptr<JSONRpcEndpoint> rpcEndpoint =
      std::make_unique<JSONRpcEndpoint>(writeIn, readOut);
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

  if (readIn != -1) {
    close(readIn);
  }

  if (writeIn != -1) {
    close(writeIn);
  }

  if (readOut != -1) {
    close(readOut);
  }

  if (writeOut != -1) {
    close(writeOut);
  }

  if (processId != -1) {
    int status;
    // wait processId to exit automatically (after exit request or exit
    // accidentally)
    waitpid(processId, &status, 0);
  }
}