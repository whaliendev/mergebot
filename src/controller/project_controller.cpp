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
void __checkPath(std::string const& pathStr) {
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

void __checkGitDir(std::string const& path) {
  const fs::path gitDirPath = fs::path(path) / ".git";
  if (!(fs::exists(gitDirPath) && fs::is_directory(gitDirPath))) {
    spdlog::warn(util::format("project path[{}] is not a valid git repo", path));
    throw AppBaseException("U1000", util::format("project path[{}] is not a valid git repo", path));
  }
}

bool __containKeys(const crow::json::rvalue& bodyJson, const std::vector<std::string>& keys) {
  for (const auto& key : keys) {
    if (!bodyJson.has(key)) {
      return false;
    }
  }
  return true;
}

void __initProject(std::string const& project, std::string const& path) {
  const fs::path homePath = fs::path(util::toabs(MBDIR));
  if (!fs::exists(homePath)) fs::create_directories(homePath);

  util::SHA1 checksum;
  checksum.update(util::format("{}-{}", project, path));
  const std::string cacheDirStr = checksum.final();
  const fs::path projCheckSumDir = homePath / cacheDirStr;
  sa::Project proj;
  proj.project = project;
  proj.path = path;
  proj.cacheDir = projCheckSumDir;
  assert(proj.project.size() && proj.path.size() && proj.cacheDir.size());
  std::vector<sa::Project> projVec{proj};

  // optimize: we distribute all the projects to 256 manifest.json and 16 mutex to reduce lock
  // contention and critical section
  const fs::path manifestPath = homePath / util::format("{}.json", cacheDirStr.substr(0, 2));

  std::lock_guard<std::mutex> manifestLock(
      MANIFEST_LOCKS[std::stoi(cacheDirStr.substr(0, 2)) % MANIFEST_LOCKS.size()]);
  if (fs::exists(manifestPath)) {
    // exists, do patch
    std::ifstream manifestFile(manifestPath.string());
    if (!json::accept(manifestFile)) {  // not valid, it's an error
      spdlog::error(
          "the content of file {} is not valid json, which means it's broken, something pretty"
          "bad happened, please check the log",
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
      auto it = std::find_if(cachedProjsVec.begin(), cachedProjsVec.end(), theSame);
      if (it != std::end(cachedProjsVec)) {
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
    spdlog::info("proj[{}] cache dir[{}] created", project, projCheckSumDir.string());
  }
}

crow::json::wvalue __doPostProject(const crow::request& req, crow::response& res) {
  // args check
  const auto body = crow::json::load(req.body);
  if (body.error() || !__containKeys(body, {"project", "path"})) {
    spdlog::warn("wrong request format");
    throw AppBaseException(ResultEnum::BAD_REQUEST);
  }
  const auto project = static_cast<std::string>(body["project"]);
  const auto path = static_cast<std::string>(body["path"]);
  __checkPath(path);
  __checkGitDir(path);
  // generate project mapping files
  __initProject(project, path);
  return {};
}

crow::json::wvalue __doPostMergeScenario(const crow::request& req, crow::response& res) {
  return {};
}
}  // namespace internal

void PostProject(const crow::request& req, crow::response& res) {
  auto internalPostProject =
      ExceptionHandlerAspect<CReqMResFuncType>(internal::__doPostProject, res);
  auto rv = internalPostProject(req, res);
  if (noerr(rv)) ResultVOUtil::return_success(res, rv);
}

void PostMergeScenario(const crow::request& req, crow::response& res) {
  auto internalPostMergeScenario =
      ExceptionHandlerAspect<CReqMResFuncType>(internal::__doPostMergeScenario, res);
  auto rv = internalPostMergeScenario(req, res);
  if (noerr(rv)) ResultVOUtil::return_success(res, rv);
}
}  // namespace server
}  // namespace mergebot
