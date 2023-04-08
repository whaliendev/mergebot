//
// Created by whalien on 04/03/23.
//

#include "mergebot/core/ResolutionManager.h"

#include <filesystem>
#include <fmt/format.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <system_error>
#include <thread>
#include <unordered_set>

#include "mergebot/core/handler/ASTBasedHandler.h"
#include "mergebot/core/handler/LLVMBasedHandler.h"
#include "mergebot/core/handler/SAHandler.h"
#include "mergebot/core/handler/StyleBasedHandler.h"
#include "mergebot/core/handler/TextBasedHandler.h"
#include "mergebot/utils/ThreadPool.h"
#include "mergebot/utils/stringop.h"

namespace mergebot {
namespace sa {
void ResolutionManager::doResolution() {
  // remember to clear running sign
  // clang-format off
  spdlog::info(R"(begin resolving conflicts...
  {{
    project: {},
    project path: {},
    merge scenario: {},
    conflict files: [
        {}
    ]
  }}
  )", Project_, ProjectPath_, MS_,
  fmt::join(ConflictFiles_.get(), ConflictFiles_.get() + FileNum_, ",\n\t"));
  // clang-format on

  // use_count 2
  const std::shared_ptr<ResolutionManager> Self = shared_from_this();
  std::thread ResolveAsyncThread(
      std::bind(&ResolutionManager::_doResolutionAsync, Self));
  ResolveAsyncThread.detach();
}

void ResolutionManager::_doResolutionAsync(
    std::shared_ptr<ResolutionManager> const &Self) {
  // mkdir MSCacheDir / resolutions
  const fs::path ResolutionDest =
      fs::path(Self->mergeScenarioPath()) / "resolutions" / "";
  if (!fs::exists(ResolutionDest)) {
    fs::create_directories(ResolutionDest);
  }

  // copy c/cpp related conflict files
  // FIX(hwa): avoid file name collision
  const fs::path ConflictDest =
      fs::path(Self->mergeScenarioPath()) / "conflicts" / "";
  std::vector<std::string> CSources = Self->_extractCppSources();
  std::transform(CSources.begin(), CSources.end(), CSources.begin(),
                 [&](const std::string_view &sv) {
                   return Self->ProjectPath_ + std::move(std::string(sv));
                 });
  std::string FileList = std::accumulate(
      std::next(CSources.begin()), CSources.end(), std::string(CSources[0]),
      [](std::string pre, std::string_view const cur) {
        return std::move(pre) + " " + std::string(cur);
      });
  spdlog::debug("conflict file list: {}", FileList);
  auto CMD = fmt::format("rsync -auLv {} {}", FileList, ConflictDest.string());
  llvm::ErrorOr<std::string> CopyResultOrErr = util::ExecCommand(CMD);
  if (!CopyResultOrErr) {
    sa::handleSAExecError(CopyResultOrErr.getError(), CMD);
  }

  // abort merge
  CMD = fmt::format("(cd {} && git merge --abort)", Self->ProjectPath_);
  llvm::ErrorOr<std::string> ResultOrErr = util::ExecCommand(CMD);
  if (!ResultOrErr) {
    sa::handleSAExecError(ResultOrErr.getError(), CMD);
  }
  spdlog::info("abort merge in project [{}], preparing to conduct analysis",
               Self->Project_);

  // checkout to specific merge commit, fast copy to dest, generate CompDB
  const fs::path OursPath = fs::path(Self->mergeScenarioPath()) / "ours";
  const fs::path TheirsPath = fs::path(Self->mergeScenarioPath()) / "theirs";
  // ours and theirs should not execute parallel, otherwise rsync and git will
  // collide
  ResolutionManager::_generateCompDB(Self, Self->MS_.ours, OursPath);
  ResolutionManager::_generateCompDB(Self, Self->MS_.theirs, TheirsPath);
  spdlog::info("CompDB for ours branch and theirs branch in "
               "project[{}] generated successfully",
               Self->Project_);

  // restore project to previous state
  // TODO(hwa): extract following piece of code to a cleanup function
  const auto RestoreCMD =
      fmt::format("(cd {} && git checkout {} && git merge {})",
                  Self->ProjectPath_, Self->MS_.ours, Self->MS_.theirs);
  llvm::ErrorOr<std::string> RestoreResultOrErr =
      util::ExecCommand(RestoreCMD, 10, 256);
  if (!RestoreResultOrErr) {
    handleSAExecError(RestoreResultOrErr.getError(), RestoreCMD);
  }
  spdlog::info("restore merge conflicts in project[{}]\n\n\n"
               "***  all the preparation work(project[{}]) done, the "
               "resolution algorithm starts  ***",
               Self->Project_, Self->Project_);

  // task queue -> thread queue -> resolved file
  //  assert(fs::exists(OursPath) && fs::exists(TheirsPath) &&
  //         fs::exists(OursPath / "compile_commands.json") &&
  //         fs::exists(TheirsPath / "compile_commands.json") &&
  //         "CompDB generation failed");
  assert(fs::exists(OursPath) && fs::exists(TheirsPath) &&
         "copy two version sources failed");
  ProjectMeta Meta{
      .Project = Self->Project_,
      .ProjectPath = Self->ProjectPath_,
      .ProjectCacheDir = Self->projectCacheDir(),
      .MS = Self->MS_,
      .MSCacheDir = Self->mergeScenarioPath(),
  };
  std::vector<std::unique_ptr<SAHandler>> Handlers;
  Handlers.push_back(std::make_unique<StyleBasedHandler>(Meta));
  Handlers.push_back(std::make_unique<ASTBasedHandler>(Meta));
  Handlers.push_back(std::make_unique<LLVMBasedHandler>(Meta));
  Handlers.push_back(std::make_unique<TextBasedHandler>(Meta));

  HandlerChain Chain(std::move(Handlers), CSources);
  Chain.handle();

  // TODO(hwa): Remember to remove running sign at last
}

void ResolutionManager::_generateCompDB(
    const std::shared_ptr<ResolutionManager> &Self,
    const std::string &CommitHash, std::string const &SourceDest) {
  // checkout to specific merge scenario
  const auto CheckoutCMD =
      fmt::format("cd {} && git checkout {}", Self->ProjectPath_, CommitHash);
  const auto CheckoutResultOrErr = util::ExecCommand(CheckoutCMD);
  if (!CheckoutResultOrErr) {
    sa::handleSAExecError(CheckoutResultOrErr.getError(), CheckoutCMD);
  }

  // fast copy to `SourceDest`
  const auto CopyCMD =
      fmt::format("rsync -rauLv {} {}", Self->ProjectPath_, SourceDest);
  const auto CopyResultOrErr = util::ExecCommand(CopyCMD);
  if (!CopyResultOrErr) {
    sa::handleSAExecError(CopyResultOrErr.getError(), CopyCMD);
  }

  // generate CompDB.
  // At this stage, we simply move the 2 compile_commands to mergescenario dir
  // TODO(hwa): Generate CompDB
  spdlog::info("Generate CompDB for {} successfully", SourceDest);
}
std::vector<std::string> ResolutionManager::_extractCppSources() {
  // get conflict files
  std::string Command =
      fmt::format("(cd {} && git --no-pager diff --name-only --diff-filter=U)",
                  ProjectPath_);
  llvm::ErrorOr<std::string> ResultOrErr = util::ExecCommand(Command);
  if (!ResultOrErr) {
    handleSAExecError(ResultOrErr.getError(), Command);
  }
  std::string Result = ResultOrErr.get();

  // get c/cpp related source files
  std::vector<std::string_view> FileNames = util::string_split(Result, "\n");
  if (FileNames.empty()) {
    spdlog::error("there is no conflict files in this project[{}]", Project_);
  }

  // clang-format off
  std::unordered_set<std::string_view> CExtensions = {
      ".h", ".hpp", ".c", ".cc", ".cp", ".C", ".cxx", ".cpp", ".c++"
  };
  // clang-format on
  std::vector<std::string> CSources;
  CSources.reserve(FileNames.size());
  for (const auto &FileName : FileNames) {
    using namespace std::literals;
    auto pos = FileName.find_last_of("."sv);
    if (pos == std::string_view::npos) {
      continue;
    }
    std::string_view Ext = FileName.substr(pos);
    if (CExtensions.count(Ext)) {
      CSources.push_back(std::string(FileName));
    }
  }
  if (CSources.empty()) {
    spdlog::warn("there is no c/cpp related conflict files in this project[{}]",
                 Project_);
  }
  return CSources;
}
} // namespace sa
} // namespace mergebot
