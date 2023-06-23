//
// Created by whalien on 04/03/23.
//

#include "mergebot/core/ResolutionManager.h"

#include <filesystem>
#include <fmt/core.h>
#include <fmt/format.h>
#include <memory>
#include <oneapi/tbb/task_group.h>
#include <oneapi/tbb/tick_count.h>
#include <spdlog/spdlog.h>
#include <system_error>
#include <thread>
#include <unordered_set>

#include "mergebot/core/handler/ASTBasedHandler.h"
#include "mergebot/core/handler/LLVMBasedHandler.h"
#include "mergebot/core/handler/SAHandler.h"
#include "mergebot/core/handler/StyleBasedHandler.h"
#include "mergebot/core/handler/TextBasedHandler.h"
#include "mergebot/core/sa_utility.h"
#include "mergebot/utils/ThreadPool.h"
#include "mergebot/utils/fileio.h"
#include "mergebot/utils/gitservice.h"
#include "mergebot/utils/stringop.h"

namespace mergebot {
namespace sa {
const std::string ResolutionManager::CompDBRelative =
    "build/compile_commands.json";

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
  } else {
    bool Success = fs::remove_all(ResolutionDest);
    if (!Success) {
      spdlog::error("fail to rm -r {}, which may cause serious problem to "
                    "output resolution result",
                    ResolutionDest.string());
    }
    fs::create_directories(ResolutionDest);
  }

  // copy c/cpp related conflict files
  // FIXME(hwa): avoid file name collision
  const fs::path ConflictDest =
      fs::path(Self->mergeScenarioPath()) / "conflicts" / "";
  std::vector<std::string> CSources = Self->_extractCppSources();
  std::transform(CSources.begin(), CSources.end(), CSources.begin(),
                 [&](const std::string_view &sv) {
                   return Self->ProjectPath_ + std::string(sv);
                 });
  std::string FileList = std::accumulate(
      std::next(CSources.begin()), CSources.end(), std::string(CSources[0]),
      [](std::string pre, std::string_view const cur) {
        return std::move(pre) + " " + std::string(cur);
      });
  spdlog::debug("conflict file list: {}", FileList);
  // FIXME(hwa): remove rsync deps
  auto CMD = fmt::format("rsync -auLv {} {}", FileList, ConflictDest.string());
  llvm::ErrorOr<std::string> CopyResultOrErr = util::ExecCommand(CMD);
  if (!CopyResultOrErr) {
    sa::handleSAExecError(CopyResultOrErr.getError(), CMD);
  }

  tbb::tick_count Start = tbb::tick_count::now();
  tbb::task_group TG;
  spdlog::info("collecting source, preparing to conduct analysis");
  CMD = fmt::format("(cd {} && git merge-base {} {})", Self->ProjectPath_,
                    Self->MS_.ours, Self->MS_.theirs);
  llvm::ErrorOr<std::string> BaseOrErr = util::ExecCommand(CMD);
  if (!BaseOrErr) {
    sa::handleSAExecError(BaseOrErr.getError(), CMD);
  }

  std::string &BaseCommitHash = BaseOrErr.get();
  // 40 is a magic number of commit hash length
  if (BaseCommitHash.length() != 40) {
    spdlog::error("merge base {} of commit {} and commit {} is illegal",
                  BaseCommitHash.substr(0, 8), Self->MS_.ours.substr(0, 8),
                  Self->MS_.theirs.substr(0, 8));
  }

  const fs::path BasePath = fs::path(Self->mergeScenarioPath()) / "base";
  const fs::path OursPath = fs::path(Self->mergeScenarioPath()) / "ours";
  const fs::path TheirsPath = fs::path(Self->mergeScenarioPath()) / "theirs";
  TG.run([&]() {
    if (BaseCommitHash.length() == 40) { // success to get base commit
      Self->MS_.base = BaseCommitHash;
      // copy to base folder and if possible, prepare CompDB
      ResolutionManager::prepareSource(Self, Self->MS_.base, BasePath);
    }
  });
  TG.run([&]() {
    ResolutionManager::prepareSource(Self, Self->MS_.ours, OursPath);
  });
  TG.run([&]() {
    ResolutionManager::prepareSource(Self, Self->MS_.theirs, TheirsPath);
  });
  TG.wait();
  tbb::tick_count End = tbb::tick_count::now();
  spdlog::info(
      "it takes {} ms to copy 3(or 2) versions' source and fine tune CompDB",
      (End - Start).seconds() * 1000);
  assert(fs::exists(OursPath) && fs::exists(TheirsPath) &&
         "copy our version's and their version's source failed");

  spdlog::info("***  all the preparation work(project[{}]) done, the "
               "resolution algorithm starts  ***",
               Self->Project_, Self->Project_);
  // task queue -> thread queue -> resolved file
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

  const fs::path RunningSign = fs::path(Self->mergeScenarioPath()) / "running";
  if (fs::exists(RunningSign)) {
    fs::remove(RunningSign);
    spdlog::info("unlock merge scenario(remove running sign)\n\n\n");
  }
}

void ResolutionManager::prepareSource(
    const std::shared_ptr<ResolutionManager> &Self,
    const std::string &CommitHash, std::string const &SourceDest) {
  // use libgit2 to read commit tree and dump to SourceDest
  bool Success = mergebot::util::dump_tree_object_to(SourceDest, CommitHash,
                                                     Self->ProjectPath_);
  if (!Success) {
    spdlog::error("fail to dump version {} to {}", CommitHash, SourceDest);
  } else {
    spdlog::info("source code of commit prepared, written to {}", CommitHash,
                 SourceDest);
  }

  // generate CompDB.
  // At this stage, we simply move the 3 compile_commands to merge scenario dir
  // and do a textual replacement
  bool fineTuned = false;
  const fs::path OrigCompDBPath = fs::path(Self->ProjectPath_) / CompDBRelative;
  if (fs::exists(OrigCompDBPath)) {
    const fs::path CurCompDBPath = fs::path(SourceDest) / CompDBRelative;
    bool copied = util::copy_file(OrigCompDBPath, CurCompDBPath);
    if (copied) {
      fineTuned = fineTuneCompDB(CurCompDBPath, SourceDest, Self->ProjectPath_);
      if (fineTuned) {
        spdlog::info("CompDB fine tuned, written to {}",
                     CurCompDBPath.string());
      }
    }
  }
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

bool ResolutionManager::fineTuneCompDB(const std::string &CompDBPath,
                                       std::string_view ProjPath,
                                       const std::string &OrigPath) {
  std::ifstream CompDB(CompDBPath);
  if (!CompDB.is_open()) {
    spdlog::error("failed to open CompDB {}", CompDBPath);
    return false;
  }

  std::stringstream ss;
  std::streambuf *FileBuf = CompDB.rdbuf();
  char Buffer[4096];
  std::size_t BytesRead;
  while ((BytesRead = FileBuf->sgetn(Buffer, sizeof(Buffer))) > 0) {
    ss.write(Buffer, BytesRead);
  }

  // remove trailing separator if it has
  std::string ProjectPath = OrigPath;
  if (ProjectPath.back() == fs::path::preferred_separator) {
    ProjectPath.pop_back();
  }
  std::string FileData = ss.str();
  re2::RE2 OriginalPath("(" + re2::RE2::QuoteMeta(ProjectPath) + ")");
  assert(OriginalPath.ok() && "fail to compile regex for CompDB");
  int ReplaceCnt = re2::RE2::GlobalReplace(&FileData, OriginalPath, ProjPath);
#ifndef NDEBUG
  if (!ReplaceCnt) {
    spdlog::warn("we replaced 0 original project path, which is weird");
  }
#endif

  util::file_overwrite_content(CompDBPath, FileData);
  return true;
}
} // namespace sa
} // namespace mergebot
