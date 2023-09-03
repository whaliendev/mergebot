//
// Created by whalien on 30/04/23.
//

#include "mergebot/core/semantic/GraphBuilder.h"

#include "mergebot/core/magic_enum_customization.h"
#include "mergebot/filesystem.h"
#include "mergebot/lsp/client.h"
#include <magic_enum.hpp>
#include <spdlog/spdlog.h>

namespace mergebot {
namespace sa {
bool GraphBuilder::build() {
  spdlog::info("building graph for {} side...", magic_enum::enum_name(S));

  bool Success = initLanguageServer();
  if (!Success) {
    spdlog::error("cannot initialize language server");
    return false;
  }

  for (std::string const &Path : SourceList) {
    processTranslationUnit(Path);
  }
  return true;
}

void GraphBuilder::processTranslationUnit(std::string_view Path) {}

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

  client = lsp::LspClient(std::move(LspEndpoint));

  std::string WorkspaceRoot =
      (fs::path(Meta.MSCacheDir) / magic_enum::enum_name(S)).string();
  auto InitializedResult = client.Initialize(WorkspaceRoot);
  if (InitializedResult.has_value()) {
    // TODO(hwa): review client capabilities
    spdlog::debug("server info: {}",
                  InitializedResult.value()["serverInfo"]["version"]);
  }
  return true;
}

GraphBuilder::~GraphBuilder() { client.Shutdown(); }
} // namespace sa
} // namespace mergebot