//
// Created by whalien on 04/03/23.
//

#include "mergebot/core/ResolutionManager.h"

#include <fcntl.h>
#include <filesystem>
#include <fmt/core.h>
#include <fmt/format.h>
#include <memory>
#include <oneapi/tbb/task_group.h>
#include <oneapi/tbb/tick_count.h>
#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <system_error>
#include <thread>
#include <unistd.h>
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

namespace detail {
/// copy conflict files to destination folder
/// \param FileList paths of conflict files
/// \param Dest destination folder
/// \return false if any file transmission failed, otherwise true
bool copyConflicts(const std::vector<std::string> &FileList,
                   std::string_view Dest) {
  bool Success = true;
  char Buffer[4096];
  ssize_t BytesRead = -1, BytesWritten = -1;

  // create dest directory if not exist
  if (mkdir(Dest.data(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
    if (errno != EEXIST) {
      spdlog::error("error creating destination directory: {}, reason: {}",
                    Dest, strerror(errno));
      return false;
    }
  }

  for (const auto &File : FileList) {
    std::string DestPath = fs::path(Dest) / fs::path(File).filename();
    int SrcFd, DestFd;
    struct stat StatBuffer;

    SrcFd = open(File.c_str(), O_RDONLY);
    if (SrcFd == -1) {
      spdlog::error("error opening source file: {}, reason: {}", File,
                    strerror(errno));
      Success = false;
      // flip flag to indicate failure, continue to copy as much as possible
      continue;
    }

    if (stat(DestPath.c_str(), &StatBuffer) == 0) {
      spdlog::debug("destination file {} already exists", DestPath);
      close(SrcFd);
      continue;
    }

    // not exist, create it
    DestFd = open(DestPath.c_str(), O_WRONLY | O_CREAT,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (DestFd == -1) {
      spdlog::error("error creating destination file: {}, error reason: {}",
                    DestPath, strerror(errno));
      Success = false;
      close(SrcFd);
      continue;
    }

    while ((BytesRead = ::read(SrcFd, Buffer, sizeof(Buffer))) > 0) {
      BytesWritten = ::write(DestFd, Buffer, BytesRead);
      if (BytesRead != BytesWritten) {
        spdlog::error("error writing to destination file: {}, error reason: {}",
                      DestPath, strerror(errno));
        Success = false;
        break;
      }
    }

    close(SrcFd);
    close(DestFd);

    if (BytesRead == -1) {
      spdlog::error("error reading from source file: {}, error reason: {}",
                    File, strerror(errno));
      Success = false;
    }
  }

  return Success;
}
} // namespace detail

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
}})", Project_, ProjectPath_, MS_.toPrettyString(),
  fmt::join(ConflictFiles_->begin(), ConflictFiles_->end(), ",\n\t"));
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

  spdlog::info("copying conflict files to destination directory {}",
               ConflictDest.string());
  bool Success = detail::copyConflicts(CSources, ConflictDest.string());
  if (!Success) {
    spdlog::error("fail to copy conflict files to conflicts directory {}",
                  ConflictDest.string());
  }

  tbb::tick_count Start = tbb::tick_count::now();
  tbb::task_group TG;
  spdlog::info("collecting source, preparing to conduct analysis...");
  auto CMD = fmt::format("(cd {} && git merge-base {} {})", Self->ProjectPath_,
                         Self->MS_.ours, Self->MS_.theirs);
  llvm::ErrorOr<std::string> BaseOrErr = utils::ExecCommand(CMD);
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
  spdlog::info("it takes {}ms to copy 3(or 2) versions' sources and fine tune "
               "CompDB\n",
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
      .CDBPath = Self->CDBPath_,
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
    spdlog::info("sources of commit {} prepared, written to {}", CommitHash,
                 SourceDest);
  }

  // Generate CompDB.
  // At this stage, we simply move the 3 compile_commands to merge scenario dir
  // and do a textual replacement
  bool copied = false;
  const fs::path DestCDBPath = fs::path(SourceDest) / CompDBRelative;
  fs::path OrigCompDBPath;
  // project-root
  if (fs::exists(fs::path(Self->ProjectPath_) / "compile_commands.json")) {
    OrigCompDBPath = fs::path(Self->CDBPath_);
  } else if (fs::exists(fs::path(Self->ProjectPath_) / CompDBRelative)) {
    // project-root/build
    OrigCompDBPath = fs::path(Self->ProjectPath_) / CompDBRelative;
  } else if (fs::exists(Self->CDBPath_)) {
    // specific location
    OrigCompDBPath = fs::path(Self->ProjectPath_) / "compile_commands.json";
  }

  if (!OrigCompDBPath.empty()) {
    spdlog::info("using compile_commands.json found at {}",
                 OrigCompDBPath.string());
    copied = util::copy_file(OrigCompDBPath, DestCDBPath);
  } else {
    spdlog::warn("no valid compile_commands.json found");
  }

  if (copied) {
    bool fineTuned =
        fineTuneCompDB(DestCDBPath, SourceDest, Self->ProjectPath_);
    if (fineTuned) {
      spdlog::info("CompDB fine tuned, written to {}", DestCDBPath.string());
    }
  }
}

std::vector<std::string> ResolutionManager::_extractCppSources() {
  // get conflict files
  std::string Command =
      fmt::format("(cd {} && git --no-pager diff --name-only --diff-filter=U)",
                  ProjectPath_);
  llvm::ErrorOr<std::string> ResultOrErr = utils::ExecCommand(Command);
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
                                       const std::string &ProjPath,
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
    spdlog::warn("0 original project path in {}, which is weird", CompDBPath);
  }
#endif

  util::file_overwrite_content(CompDBPath, FileData);
  return true;
}
} // namespace sa
} // namespace mergebot
