//
// Created by whalien on 09/02/23.
//
#include "project_controller.h"

#include <crow/http_response.h>
#include <crow/json.h>
#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>
#include <vector>

#include "../core/model/Project.h"
#include "../globals.h"
#include "../server/utility.h"
#include "exception_handler_aspect.h"
#include "mergebot/filesystem.h"
#include "mergebot/utils/fileio.h"
#include "mergebot/utils/format.h"
#include "mergebot/utils/pathop.h"
#include "mergebot/utils/sha1.h"

namespace mergebot {
namespace server {
namespace internal {
using json = nlohmann::json;
void _checkPath(std::string const& pathStr) {
  const fs::path dirPath = pathStr;
  const auto rwPerms = static_cast<int>(
      (fs::status(dirPath).permissions() & (fs::perms::owner_read | fs::perms::owner_write)));
  if (fs::exists(dirPath)) {
    if (!fs::is_directory(dirPath) || !rwPerms) {
      spdlog::warn(util::format("no permission to access path [{}]", pathStr));
      throw AppBaseException("U1000", util::format("no permission to access path [{}]", pathStr));
    }
  } else {
    spdlog::warn(util::format("project path [{}] doesn't exist", pathStr));
    throw AppBaseException("U1000", util::format("project path [{}] doesn't exist", pathStr));
  }
}

void _checkGitDir(std::string const& path) {
  const fs::path gitDirPath = fs::path(path) / ".git";
  if (!(fs::exists(gitDirPath) && fs::is_directory(gitDirPath))) {
    spdlog::warn(util::format("project path[{}] is not a valid git repo", path));
    throw AppBaseException("U1000", util::format("project path[{}] is not a valid git repo", path));
  }
}

bool _containKeys(const crow::json::rvalue& bodyJson, const std::vector<std::string>& keys) {
  for (const auto& key : keys) {
    if (!bodyJson.has(key)) {
      return false;
    }
  }
  return true;
}

std::string _calcProjChecksum(std::string const& project, std::string const& path) {
  util::SHA1 checksum;
  checksum.update(util::format("{}-{}", project, path));
  return checksum.final();
}

void _initProject(std::string const& project, std::string const& path) {
  const fs::path homePath = fs::path(util::toabs(MBDIR));
  if (!fs::exists(homePath)) fs::create_directories(homePath);

  const std::string cacheDirStr = _calcProjChecksum(project, path);
  const fs::path projCheckSumDir = homePath / cacheDirStr;
  sa::Project proj = {.project = project, .path = path, .cacheDir = projCheckSumDir};
  assert(proj.project.size() && proj.path.size() && proj.cacheDir.size());
  std::vector<sa::Project> projVec{proj};

  // optimize: we distribute all the projects to 256 manifest.json and use 16 mutexes to reduce lock
  // contention and critical section
  const fs::path manifestPath =
      homePath / util::format("manifest-{}.json", cacheDirStr.substr(0, 2));

  std::lock_guard<std::mutex> manifestLock(
      MANIFEST_LOCKS[std::stoi(cacheDirStr.substr(0, 2), nullptr, 16) % MANIFEST_LOCKS.size()]);
  if (fs::exists(manifestPath)) {
    // exists, do patch
    std::ifstream manifestFile(manifestPath.string());
    if (!json::accept(manifestFile)) {  // not valid, it's an reader-writer parallel error
      spdlog::error(
          "the content of file {} is not valid json, which means it's broken. there may be "
          "a reader-writer synchronization problem",
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
        spdlog::info("proj[{}] is already in manifest.json, we'll do nothing", proj.project);
      } else {
        cachedProjsVec.push_back(std::move(proj));
      }
      json projsToWrite = cachedProjsVec;
      util::file_overwrite_content(manifestPath, projsToWrite.dump(2));
    }
  } else {  // not exist
    const json projVecJson = projVec;
    util::file_overwrite_content(manifestPath, projVecJson.dump(2));
  }

  if (!fs::exists(projCheckSumDir)) {
    fs::create_directory(projCheckSumDir);
    // note that at this time proj may be moved.
    spdlog::info("proj[{}] cache dir[{}] created", project, projCheckSumDir.string());
  }
}

crow::json::wvalue _doPostProject(const crow::request& req, [[maybe_unused]] crow::response& res) {
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
  return util::ExecCommand(util::format("(cd {} && git cat-file -t {})", path, hash).c_str()) ==
         "commit";
}

void _checkMSValidity(crow::json::rvalue const& ms, std::string const& path) {
  const auto oursHash = static_cast<std::string>(ms["ours"]);
  const auto theirsHash = static_cast<std::string>(ms["theirs"]);
  if (oursHash.length() != 40 || theirsHash.length() != 40) {
    throw AppBaseException("C1000", "commit hash is not valid");
  }
  if (!_isValidCommitHash(oursHash, path)) {
    spdlog::error(util::format("there is no commit obj corresponding to hash[{}]", oursHash));
    throw AppBaseException("C1000", "the hash is not a valid commit object id in this project");
  }
  if (!_isValidCommitHash(theirsHash, path)) {
    spdlog::error(util::format("there is no commit obj corresponding to hash[{}]", theirsHash));
    throw AppBaseException("C1000", "the hash is not a valid commit object id in this project");
  }
}

void _goResolve(std::string const& project, std::string const& path, std::string const& ours,
                std::string const& theirs) {}

void _handleMergeScenario(std::string const& project, std::string const& path,
                          crow::json::rvalue const& ms) {
  const std::string cacheDirCheckSum = _calcProjChecksum(project, path);
  const int lockIdx = std::stoi(cacheDirCheckSum.substr(0, 2), nullptr, 16) % MANIFEST_LOCKS.size();
  std::mutex& manifestLock = MANIFEST_LOCKS[lockIdx];
  fs::path manifestPath = fs::path(util::toabs(MBDIR)) /
                          util::format("manifest-{}.json", cacheDirCheckSum.substr(0, 2));
  std::lock_guard<std::mutex> manifestGuard(manifestLock);
  std::ifstream manifestFS(manifestPath.string());
  if (!json::accept(manifestFS, true)) {
    spdlog::error(
        "the content of file {} is not valid json, which means it's broken. there may be a "
        "reader-writer synchronization problem");
    throw AppBaseException(
        "S1000", util::format("MBSA目录下映射文件{}格式非法，可能是一个读者写者同步bug，请检查日志",
                              manifestPath.string()));
  } else {
    std::ifstream localManifestFS(manifestPath.string());
    json manifestJson = json::parse(localManifestFS);
    std::vector<sa::Project> projList = manifestJson;
    auto theSame = [&](const sa::Project& cachedProj) {
      return cachedProj.cacheDir == cacheDirCheckSum;
    };
    std::vector<sa::Project>::iterator it = std::find_if(projList.begin(), projList.end(), theSame);
    if (projList.end() != it) {
      // find it!
      std::string ours = static_cast<std::string>(ms["ours"]);
      std::string theirs = static_cast<std::string>(ms["theirs"]);
      std::string base =
          util::ExecCommand(util::format("git merge-base {} {}", ours, theirs).c_str());
      if (base.length() != 40) base = "";
      std::string name = util::format("{}-{}", ours.substr(0, 6), theirs.substr(0, 6));
      sa::MergeScenario msCrafted = {.name = name, .ours = ours, .theirs = theirs, .base = base};
      auto msIt = std::find_if(it->mss.begin(), it->mss.end(),
                               [&](sa::MergeScenario const& ms) { return ms.name == name; });
      {
        std::lock_guard<std::mutex> manifestLockGuard(manifestLock);
        if (msIt == it->mss.end()) {  // not exists before
          it->mss.push_back(std::move(msCrafted));
          json jsonToDump = projList;
          util::file_overwrite_content(manifestPath, jsonToDump.dump(2));
          _goResolve(project, path, ours, theirs);
        } else {  // check if the algorithm is running
          // not running, go resolve
          // running, return
          // TODO(hwa): add check logic
        }
      }
    } else {
      // something wrong
      spdlog::warn(util::format(
          "use /project api to post project to mergebot first before post merge scenario"));
      throw AppBaseException("C1000", "请先调用/project api将项目信息添加到mergebot中再调用此api");
    }
  }
}

crow::json::wvalue _doPostMergeScenario(const crow::request& req,
                                        [[maybe_unused]] crow::response& res) {
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
  _handleMergeScenario(project, path, body["ms"]);
  return {};
}
}  // namespace internal

void PostProject(const crow::request& req, crow::response& res) {
  auto internalPostProject =
      ExceptionHandlerAspect<CReqMResFuncType>(internal::_doPostProject, res);
  auto rv = internalPostProject(req, res);
  if (noerr(rv)) ResultVOUtil::return_success(res, rv);
}

void PostMergeScenario(const crow::request& req, crow::response& res) {
  auto internalPostMergeScenario =
      ExceptionHandlerAspect<CReqMResFuncType>(internal::_doPostMergeScenario, res);
  auto rv = internalPostMergeScenario(req, res);
  if (noerr(rv)) ResultVOUtil::return_success(res, rv);
}
}  // namespace server
}  // namespace mergebot
