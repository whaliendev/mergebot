//
// Created by whalien on 09/02/23.
//
#include "project_controller.h"

#include <crow/http_response.h>
#include <crow/json.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <nlohmann/json.hpp>
#include <unordered_set>
#include <vector>

#include "../core/ResolutionManager.h"
#include "../core/model/Project.h"
#include "../globals.h"
#include "../utility.h"
#include "exception_handler_aspect.h"
#include "llvm/Support/ErrorOr.h"
#include "mergebot/filesystem.h"
#include "mergebot/utils/fileio.h"
#include "mergebot/utils/format.h"
#include "mergebot/utils/pathop.h"
#include "mergebot/utils/sha1.h"
#include "mergebot/utils/stringop.h"

namespace mergebot {
namespace server {
namespace internal {
using json = nlohmann::json;
void _checkPath(std::string const& pathStr) {
  const fs::path dirPath = pathStr;
  const auto rwPerms =
      static_cast<int>((fs::status(dirPath).permissions() &
                        (fs::perms::owner_read | fs::perms::owner_write)));
  if (fs::exists(dirPath)) {
    if (!fs::is_directory(dirPath) || !rwPerms) {
      spdlog::warn(util::format("no permission to access path [{}]", pathStr));
      throw AppBaseException(
          "U1000", util::format("no permission to access path [{}]", pathStr));
    }
  } else {
    spdlog::warn(util::format("project path [{}] doesn't exist", pathStr));
    throw AppBaseException(
        "U1000", util::format("project path [{}] doesn't exist", pathStr));
  }
}

void _checkGitDir(std::string const& path) {
  const fs::path gitDirPath = fs::path(path) / ".git";
  if (!(fs::exists(gitDirPath) && fs::is_directory(gitDirPath))) {
    spdlog::warn(
        util::format("project path[{}] is not a valid git repo", path));
    throw AppBaseException(
        "U1000",
        util::format("project path[{}] is not a valid git repo", path));
  }
}

bool _containKeys(const crow::json::rvalue& bodyJson,
                  const std::vector<std::string>& keys) {
  for (const auto& key : keys) {
    if (!bodyJson.has(key)) {
      return false;
    }
  }
  return true;
}

std::string _calcProjChecksum(std::string const& project,
                              std::string const& path) {
  util::SHA1 checksum;
  checksum.update(util::format("{}-{}", project, path));
  return checksum.final();
}

void _initProject(std::string const& project, std::string const& path) {
  const fs::path homePath = fs::path(mergebot::util::toabs(MBDIR));
  if (!fs::exists(homePath)) fs::create_directories(homePath);

  const std::string cacheDirStr = _calcProjChecksum(project, path);
  const fs::path projCheckSumDir = homePath / cacheDirStr;
  sa::Project proj = {
      .project = project, .path = path, .cacheDir = projCheckSumDir};
  assert(proj.project.size() && proj.path.size() && proj.cacheDir.size());
  std::vector<sa::Project> projVec{proj};

  // optimize: we distribute all the projects to 256 manifest.json and use 16
  // mutexes to reduce lock contention and critical section
  const fs::path manifestPath =
      homePath / util::format("manifest-{}.json", cacheDirStr.substr(0, 2));

  std::lock_guard<std::mutex> manifestLock(
      MANIFEST_LOCKS[std::stoi(cacheDirStr.substr(0, 2), nullptr, 16) %
                     MANIFEST_LOCKS.size()]);
  if (fs::exists(manifestPath)) {
    // exists, do patch
    spdlog::info("manifest file [{}] already exits, we'll do a patch to it",
                 manifestPath.string());
    std::ifstream manifestFile(manifestPath.string());
    if (!json::accept(
            manifestFile)) {  // not valid, it's an reader-writer parallel error
      spdlog::error(
          "the content of file {} is not valid json, which means it's broken. "
          "there may be a reader-writer synchronization problem",
          manifestPath.string());
      throw AppBaseException("S1000", "MBSA目录下映射文件格式非法，请检查日志");
    } else {
      // Note that json::accept has seeked to the end of outer manifestFile,
      // so here we need to open a new file descriptor
      std::ifstream localManifestFile(manifestPath.string());
      json cachedProjs = json::parse(localManifestFile);
      std::vector<sa::Project> cachedProjsVec = cachedProjs;
      auto theSame = [&](const sa::Project& cachedProj) {
        return cachedProj.cacheDir == proj.cacheDir;
      };
      if (std::find_if(cachedProjsVec.begin(), cachedProjsVec.end(), theSame) !=
          std::end(cachedProjsVec)) {
        spdlog::info("proj[{}] is already in manifest.json, we'll do nothing",
                     proj.project);
      } else {
        cachedProjsVec.push_back(std::move(proj));
      }
      json projsToWrite = cachedProjsVec;
      util::file_overwrite_content(manifestPath, projsToWrite.dump(2));
    }
  } else {  // not exist
    spdlog::info("manifest file [{}] doesn't exist, we'll create one",
                 manifestPath.string());
    const json projVecJson = projVec;
    util::file_overwrite_content(manifestPath, projVecJson.dump(2));
  }

  if (!fs::exists(projCheckSumDir)) {
    fs::create_directory(projCheckSumDir);
    // note that at this time proj may be moved.
    spdlog::info("proj[{}] cache dir[{}] created", project,
                 projCheckSumDir.string());
  }
}

crow::json::wvalue _doPostProject(const crow::request& req,
                                  [[maybe_unused]] crow::response& res) {
  // args check
  const auto body = crow::json::load(req.body);
  if (body.error() || !_containKeys(body, {"project", "path"})) {
    spdlog::error("the format of request body data is illegal");
    throw AppBaseException(ResultEnum::BAD_REQUEST);
  }
  const auto project = static_cast<std::string>(body["project"]);
  const auto path = static_cast<std::string>(body["path"]);
  _checkPath(path);
  _checkGitDir(path);
  // generate project mapping files
  _initProject(project, path);
  return {};
}

bool _isValidCommitHash(std::string const& hash, std::string const& path) {
  std::string command =
      util::format("(cd {} && git cat-file -t {})", path, hash);
  llvm::ErrorOr<std::string> resultOrErr = util::ExecCommand(command);
  if (!resultOrErr) handleServerExecError(resultOrErr.getError(), command);
  return resultOrErr.get() == "commit";
}

void _checkMSValidity(crow::json::rvalue const& ms, std::string const& path) {
  const auto oursHash = static_cast<std::string>(ms["ours"]);
  const auto theirsHash = static_cast<std::string>(ms["theirs"]);
  if (oursHash.length() != 40 || theirsHash.length() != 40) {
    throw AppBaseException("C1000", "commit hash is not valid");
  }
  if (!_isValidCommitHash(oursHash, path)) {
    spdlog::error(util::format(
        "there is no commit obj corresponding to hash[{}] in git repo[{}]",
        oursHash, path));
    throw AppBaseException(
        "C1000",
        util::format("the hash is not a valid commit object id in {}", path));
  }
  if (!_isValidCommitHash(theirsHash, path)) {
    spdlog::error(util::format(
        "there is no commit obj corresponding to hash[{}]", theirsHash));
    throw AppBaseException(
        "C1000",
        util::format("the hash is not a valid commit object id in {}", path));
  }
}

void _goResolve(std::string project, std::string path, std::string ours,
                std::string theirs, std::string base, crow::response& res) {
  // collect conflict files in path of project
  std::string command =
      util::format("(cd {} && git diff --name-only --diff-filter=U)", path);
  llvm::ErrorOr<std::string> resultOrErr = util::ExecCommand(command);
  if (!resultOrErr) handleServerExecError(resultOrErr.getError(), command);
  std::string result = resultOrErr.get();

  // get c/cpp related source files
  std::vector<std::string_view> fileNames = util::string_split(result, "\n");
  if (fileNames.size() == 0) {
    throw AppBaseException("U1000",
                           util::format("there is no conflict C/C++ source "
                                        "file in project[{}] with path[{}]",
                                        project, path));
  }
  // clang-format off
  std::unordered_set<std::string_view> cppExtensions = {".h", ".hpp",
                            ".c", ".cc", ".cp", ".C", ".cxx", ".cpp", ".c++"};
  // clang-format on
  std::vector<std::string_view> cppSources;
  cppSources.reserve(fileNames.size());
  for (const auto& fileName : fileNames) {
    using namespace std::literals;
    auto pos = fileName.find_last_of("."sv);
    if (pos == std::string_view::npos) continue;
    std::string_view ext = fileName.substr(pos);
    if (cppExtensions.count(ext)) cppSources.push_back(fileName);
  }
  if (fileNames.size() && !cppSources.size()) {
    spdlog::info(
        "current project[{}] has {} conflict files, but none of them are c/cpp "
        "sources, we cannot handle them at this stage",
        project, fileNames.size());
    throw AppBaseException(
        "U1000", util::format("当前项目[{}]有{}个冲突文件，但由于都不是C/"
                              "C++相关的源文件，mergebot-sa当前阶段无法处理",
                              project, fileNames.size()));
  }

  spdlog::info(
      "current project[{}, path: {}] has {} conflict files, {} of them are "
      "C/C++ related sources",
      project, path, fileNames.size(), cppSources.size());
  std::unique_ptr<std::string[]> conflictFiles =
      std::make_unique<std::string[]>(cppSources.size());
  std::transform(
      cppSources.begin(), cppSources.end(), conflictFiles.get(),
      [](const std::string_view& fileName) { return std::string(fileName); });
  sa::MergeScenario ms(ours, theirs, base);
  // construct ResolutionManager, enable shared from this
  std::shared_ptr<sa::ResolutionManager> resolutionManager =
      std::make_shared<sa::ResolutionManager>(
          std::move(project), std::move(path), std::move(ms),
          std::move(conflictFiles), cppSources.size());
  // call its async doResolution method to do resolution
  resolutionManager->doResolution();
}

/// \brief set merge scenario resolution algorithm running sign in
/// projDir/msName dir. Note that this method should be called in a thread-safe
/// context. \param projDir project cache dir \param msName merge scenario name
void _setRunningSign(std::string const& projDir, std::string const& msName) {
  fs::path msPath = fs::path(projDir) / msName;
  // clang-format off
  assert(fs::exists(msPath) &&
         util::format(
             "the merge scenario path[{}] should be created before setting running sign", msPath
         ).c_str());
  // clang-format on
  util::file_overwrite_content(msPath / "running", std::to_string(1));
}

void _handleMergeScenario(std::string const& project, std::string const& path,
                          crow::json::rvalue const& ms, crow::response& res) {
  const std::string cacheDirCheckSum = _calcProjChecksum(project, path);
  fs::path manifestPath =
      fs::path(util::toabs(MBDIR)) /
      util::format("manifest-{}.json", cacheDirCheckSum.substr(0, 2));
  std::ifstream manifestFS(manifestPath.string());
  if (!json::accept(manifestFS, true)) {
    spdlog::error(
        "the content of file {} is not valid json, which means it's broken. "
        "there may be a reader-writer synchronization problem");
    throw AppBaseException(
        "S1000", util::format("MBSA目录下映射文件{}"
                              "格式非法，可能是一个读者写者同步bug，请检查日志",
                              manifestPath.string()));
  }
  std::ifstream localManifestFS(manifestPath.string());
  json manifestJson = json::parse(localManifestFS);
  std::vector<sa::Project> projList = manifestJson;
  auto theSame = [&](const sa::Project& cachedProj) {
    return fs::path(cachedProj.cacheDir).filename() == cacheDirCheckSum;
  };
  std::vector<sa::Project>::iterator it =
      std::find_if(projList.begin(), projList.end(), theSame);
  if (projList.end() == it) {
    // didn't find it, something wrong
    spdlog::warn(
        util::format("use /project api to post project to mergebot first "
                     "before post merge scenario"));
    throw AppBaseException(
        "C1000", "请先调用/project api将项目信息添加到mergebot中再调用此api");
  }

  // find it
  std::string ours = static_cast<std::string>(ms["ours"]);
  std::string theirs = static_cast<std::string>(ms["theirs"]);
  std::string cmd = mergebot::util::format("(cd {} && git merge-base {} {})",
                                           path, ours, theirs);
  llvm::ErrorOr<std::string> resultOrErr = mergebot::util::ExecCommand(cmd);
  if (!resultOrErr) handleServerExecError(resultOrErr.getError(), cmd);
  std::string base = resultOrErr.get();
  if (base.length() != 40) {
    base = "";
    spdlog::warn(
        "base commit of ours[{}] and theirs[{}] branches doesn't exist", ours,
        theirs);
  }
  std::string name =
      util::format("{}-{}", ours.substr(0, 6), theirs.substr(0, 6));
  sa::MergeScenario msCrafted(ours, theirs, base);
  auto msIt = std::find_if(
      (it->mss).begin(), (it->mss).end(),
      [&](sa::MergeScenario const& ms) { return ms.name == name; });

  const int lockIdx = std::stoi(cacheDirCheckSum.substr(0, 2), nullptr, 16) %
                      MANIFEST_LOCKS.size();
  std::mutex& manifestLock = MANIFEST_LOCKS[lockIdx];
  {
    std::lock_guard<std::mutex> manifestLockGuard(manifestLock);
    if (msIt == (it->mss).end()) {  // not exists before
      // note that in the following code, projList will be moved to basic_json
      // and msCrafted will be moved to projList, so we need to back up them
      // here.
      spdlog::info(
          "this merge scenario[{}] of current project[{}] doesn't exist "
          "before, we'll add it and start the resolution algorithm",
          msCrafted, project);
      std::string projFoundCacheDir = it->cacheDir;
      std::string msName = msCrafted.name;

      it->mss.push_back(std::move(msCrafted));
      json jsonToDump = projList;
      util::file_overwrite_content(manifestPath, jsonToDump.dump(2));
      fs::path msPath = fs::path(projFoundCacheDir) / msName;
      fs::create_directories(msPath);
      _setRunningSign(projFoundCacheDir, msName);
      // Remember to clear running sign manually after we resolved all the
      // conflicts.
    } else {  // check if the algorithm is running
      // if it's running, return
      spdlog::info(
          "this merge scenario[{}] of project[{}] already exist in the "
          "resolution set, we'll check if the algorithm is running",
          msCrafted, project);
      fs::path runningSignFile =
          fs::path(it->cacheDir) / msIt->name / "running";
      if (fs::exists(runningSignFile) &&
          util::file_get_content(runningSignFile) == "1") {
        throw AppBaseException("C1000",
                               "当前项目当前合并场景的冲突解决算法在运行中");
      } else {
        spdlog::info(
            util::format("the resolution algorithm of project[{}] is not "
                         "running, we'll start it soon",
                         project));
      }
    }
  }
  _goResolve(project, path, ours, theirs, base, res);
}

crow::json::wvalue _doPostMergeScenario(const crow::request& req,
                                        crow::response& res) {
  const auto body = crow::json::load(req.body);
  if (body.error() || !_containKeys(body, {"project", "path", "ms"})) {
    spdlog::error("the format of request body data is illegal");
    throw AppBaseException(ResultEnum::BAD_REQUEST);
  }
  const auto project = static_cast<std::string>(body["project"]);
  const auto path = static_cast<std::string>(body["path"]);
  _checkPath(path);
  _checkGitDir(path);
  _checkMSValidity(body["ms"], path);
  _handleMergeScenario(project, path, body["ms"], res);
  return {};
}
}  // namespace internal

void PostProject(const crow::request& req, crow::response& res) {
  auto internalPostProject =
      ExceptionHandlerAspect<CReqMResFuncType>(internal::_doPostProject, res);
  auto rv = internalPostProject(req, res);
  if (!err(rv)) ResultVOUtil::return_success(res, rv);
}

void PostMergeScenario(const crow::request& req, crow::response& res) {
  auto internalPostMergeScenario = ExceptionHandlerAspect<CReqMResFuncType>(
      internal::_doPostMergeScenario, res);
  auto rv = internalPostMergeScenario(req, res);
  if (!err(rv)) ResultVOUtil::return_success(res, rv);
}
}  // namespace server
}  // namespace mergebot
