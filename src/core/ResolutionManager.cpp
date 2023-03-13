//
// Created by whalien on 04/03/23.
//

#include "ResolutionManager.h"

#include "../utility.h"

#include "mergebot/utils/ThreadPool.h"
#include <fmt/format.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <thread>

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
  fmt::join(ConflictFiles_.get(), ConflictFiles_.get() + FileNum_, ",\n\t\t"));
  // clang-format on

  // use_count 2
  const std::shared_ptr<ResolutionManager> Self = shared_from_this();
  std::thread ResolveAsyncThread(
      std::bind(&ResolutionManager::_doResolutionAsync, Self));
  ResolveAsyncThread.detach();
}

void ResolutionManager::_doResolutionAsync(
    std::shared_ptr<ResolutionManager> const &Self) {
  // abort merge
  const auto CMD =
      fmt::format("(cd {} && git merge --abort)", Self->ProjectPath_);
  llvm::ErrorOr<std::string> ResultOrErr = util::ExecCommand(CMD);
  if (!ResultOrErr)
    sa::handleSAExecError(ResultOrErr.getError(), CMD);
  spdlog::info(
      fmt::format("abort merge in project [{}], preparing to conduct analysis",
                  Self->Project_));

  // checkout to specific merge commit, fast copy to dest, generate CompDB
  const fs::path OursPath = fs::path(Self->mergeScenarioPath()) / "ours";
  const fs::path TheirsPath = fs::path(Self->mergeScenarioPath()) / "theirs";
  // ours and theirs should not execute parallel, or rsync and git will collide
  ResolutionManager::_generateCompDB(Self, Self->MS_.ours, OursPath);
  ResolutionManager::_generateCompDB(Self, Self->MS_.theirs, TheirsPath);
  spdlog::info(fmt::format("CompDB for ours branch and theirs branch in "
                           "project[{}] generated successfully",
                           Self->Project_));

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
  spdlog::info(
      fmt::format("restore merge conflicts in project[{}]\n\n\n"
                  "***  all the preparation work(project[{}]) done, the "
                  "resolution algorithm starts  ***",
                  Self->Project_, Self->Project_));

  // task queue -> thread queue -> resolved file
  //  assert(fs::exists(OursPath) && fs::exists(TheirsPath) &&
  //         fs::exists(OursPath / "compile_commands.json") &&
  //         fs::exists(TheirsPath / "compile_commands.json") &&
  //         "CompDB generation failed");
  assert(fs::exists(OursPath) && fs::exists(TheirsPath) &&
         "copy two version sources failed");
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
  if (Self->ProjectPath_.back() == '/') {
    Self->ProjectPath_.pop_back();
  }
  const auto CopyCMD =
      fmt::format("rsync -rauLv {}/ {}", Self->ProjectPath_, SourceDest);
  spdlog::info(fmt::format("copy sources from {} to {}", Self->ProjectPath_,
                           SourceDest));
  const auto CopyResultOrErr = util::ExecCommand(CopyCMD);
  if (!CopyResultOrErr)
    sa::handleSAExecError(CopyResultOrErr.getError(), CopyCMD);

  // generate CompDB.
  // At this stage, we simply move the 2 compile_commands to mergescenario dir
  // TODO(hwa): Generate CompDB
  spdlog::info("Generate CompDB for {} successfully", SourceDest);
}
} // namespace sa
} // namespace mergebot
