//
// Created by whalien on 06/05/23.
//

#include <spdlog/spdlog.h>

#include <system_error>

#include "mergebot/controller/app_exception.h"
#include "mergebot/core/model/Project.h"
#include "mergebot/filesystem.h"
#include "mergebot/globals.h"
#include "mergebot/server/result_vo_utils.h"
#include "mergebot/utils/fileio.h"
#include "mergebot/utils/format.h"
#include "mergebot/utils/gitservice.h"
#include "mergebot/utils/pathop.h"
#include "mergebot/utils/sha1.h"

namespace mergebot {
namespace server {
namespace utils {
bool checkMSMetadata(const std::string& project, const std::string& path,
                     const sa::MergeScenario& ms) {
  const std::string cacheDirCheckSum = utils::calcProjChecksum(project, path);
  const fs::path projectCacheDir =
      fs::path(util::toabs(MBDIR)) / cacheDirCheckSum;
  const fs::path manifestPath =
      fs::path(util::toabs(MBDIR)) /
      fmt::format("manifest-{}.json", cacheDirCheckSum.substr(0, 2));
  const fs::path msCacheDir = projectCacheDir / ms.name;
  if (!fs::exists(projectCacheDir) || !fs::exists(msCacheDir) ||
      !fs::exists(manifestPath)) {
    spdlog::error("ms metadata[{}] doesn't exist in manifest file[{}]", ms.name,
                  manifestPath.c_str());
    throw AppBaseException("C1000",
                           fmt::format("合并场景[{}]元数据不存在于manifest文件["
                                       "{}]中，请先调用ms api启动冲突解决算法",
                                       ms.name, manifestPath.c_str()));
  }
  auto [fd, lck] = mergebot::utils::lockRDFD(manifestPath.c_str());
  if (fd == -1) {
    spdlog::error("unable to acquire read lock for manifest file[{}]",
                  manifestPath.c_str());
    return false;
  }
  FILE* file = fdopen(fd, "r");
  if (!file) {
    spdlog::error(
        "unable to convert file descriptor to FILE* object, reason: {}",
        strerror(errno));
    close(fd);
    return false;
  }
  nlohmann::json projsListJSON = nlohmann::json::parse(file);
  std::vector<sa::Project> projsList = projsListJSON;
  auto projIt = std::find_if(
      projsList.begin(), projsList.end(), [&](const sa::Project& proj) {
        return fs::path(proj.cacheDir).filename() == cacheDirCheckSum;
      });
  if (projIt == projsList.end()) {
    mergebot::utils::unlockFD(manifestPath.c_str(), fd, lck);
    fclose(file);
    throw AppBaseException(
        "C1000",
        fmt::format("项目元信息不存在，请先调用ms api添加项目[{}]元信息",
                    project));
  }
  auto msIt = std::find_if(
      projIt->mss.begin(), projIt->mss.end(),
      [&](const sa::MergeScenario& msItem) { return msItem.name == ms.name; });
  if (msIt == projIt->mss.end()) {
    mergebot::utils::unlockFD(manifestPath.c_str(), fd, lck);
    fclose(file);
    throw AppBaseException(
        "C1000",
        fmt::format(
            "合并场景元信息不存在，请先调用ms api添加合并场景[{}]元信息",
            ms.name));
  }
  return true;
}

void checkConflictFile(const std::string& project, const std::string& path,
                       const sa::MergeScenario& ms, const std::string& file) {
  fs::path filePath = fs::path(path) / file;
  if (!fs::exists(filePath)) {
    spdlog::info(
        "file[{}] to get resolution results don't exist in project[{}]", file,
        path);
    throw AppBaseException(
        "C1000",
        fmt::format("待获取冲突消解结果的文件[{}]不存在于Git仓库中", file));
  }

  const std::string cacheDirCheckSum = utils::calcProjChecksum(project, path);
  const fs::path projectCacheDir =
      fs::path(util::toabs(MBDIR)) / cacheDirCheckSum;
  const fs::path msCacheDir = projectCacheDir / ms.name;
  const fs::path conflictsFile =
      msCacheDir / "conflicts" / "conflict-sources.txt";
  assert(fs::exists(projectCacheDir) && fs::exists(msCacheDir) &&
         "ms cache dir should exist before getting resolution result");
  assert(fs::exists(conflictsFile) &&
         "conflict source should exist before getting resolution");
  std::string conflictFiles = mergebot::util::file_get_content(conflictsFile);
  std::vector<std::string_view> conflictFileList =
      util::string_split(conflictFiles, "\n");
  bool exists = std::any_of(
      conflictFileList.begin(), conflictFileList.end(),
      [&](const std::string_view conflictFile) {
        std::error_code ec;
        if (fs::path(file).is_relative()) {
          bool theSame = fs::equivalent(fs::path(path) / file,
                                        fs::path(path) / conflictFile, ec);
          if (ec) {
            return false;
          }
          return theSame;
        }
        bool theSame =
            fs::equivalent(fs::path(path) / conflictFile, fs::path(file), ec);
        if (ec) {
          return false;
        }
        return theSame;
      });
  if (!exists) {
    throw AppBaseException(
        "C1000", fmt::format("文件{}不是当前项目{}当前合并场景{}下的冲突文件",
                             file, path, ms.name));
  }
}

void checkPath(std::string const& pathStr) {
  const fs::path dirPath = pathStr;
  const auto rwPerms =
      static_cast<int>((fs::status(dirPath).permissions() &
                        (fs::perms::owner_read | fs::perms::owner_write)));
  if (fs::exists(dirPath)) {
    if (!fs::is_directory(dirPath) || !rwPerms) {
      spdlog::warn("no permission to access path [{}]", pathStr);
      throw AppBaseException("U1000", fmt::format("无权限访问 [{}]", pathStr));
    }
  } else {
    spdlog::warn("project path [{}] doesn't exist", pathStr);
    throw AppBaseException("U1000",
                           fmt::format("项目路径[{}] 不存在", pathStr));
  }
}

void checkGitRepo(std::string const& path) {
  const fs::path gitDirPath = fs::path(path) / ".git";
  const auto repoPtr = util::GitRepository::create(gitDirPath.c_str());
  if (!repoPtr) {
    spdlog::warn("project path[{}] is not a valid git repo", path);
    throw AppBaseException("U1000",
                           fmt::format("路径[{}]不是一个git仓库", path));
  }
}

bool containKeys(const crow::json::rvalue& bodyJson,
                 const std::vector<std::string>& keys) {
  for (const auto& key : keys) {
    if (!bodyJson.has(key)) {
      return false;
    }
  }
  return true;
}

std::string calcProjChecksum(std::string const& project,
                             std::string const& path) {
  util::SHA1 checksum;
  std::string pathMut = path;
  if (pathMut.back() == fs::path::preferred_separator) {
    pathMut.pop_back();
  }
  checksum.update(fmt::format("{}-{}", project, pathMut));
  return checksum.final();
}

std::string validateAndCompleteRevision(const std::string& revision,
                                        const std::string& projectPath) {
  const auto fullHash = util::commit_hash_of_rev(revision, projectPath);
  if (!fullHash.has_value()) {
    throw AppBaseException("C1000",
                           fmt::format("revision name {}不合法", revision));
  }
  return fullHash.value();
}
}  // namespace utils

void handleServerExecError(std::error_code err, std::string_view cmd) {
  if (err == std::errc::timed_out) {
    throw server::AppBaseException(
        "S1000", mergebot::util::format("timeout to executing {}", cmd));
  } else if (err == std::errc::interrupted) {
    throw server::AppBaseException(
        "S1000",
        mergebot::util::format("cmd [{}] accidentally interrupted", cmd));
  } else if (err == std::errc::io_error) {
    throw server::AppBaseException(
        "S1000", mergebot::util::format(
                     "open popen or waitpid failed for cmd [{}]", cmd));
  } else {
    throw server::AppBaseException(
        "S1000",
        mergebot::util::format("cmd [{}] accidentally exited with exit code {}",
                               cmd, err.value()));
  }
}

namespace ResultEnum {
// in C++, const vars at file scope is static linked unlike those in C.
const server::Result NO_ROUTE_MATCH(
    "C0001", "不存在匹配的路由记录，请检查请求路径或请求方法");
const server::Result BAD_REQUEST("C0002", "请求格式异常或参数错误");
}  // namespace ResultEnum

namespace ResultVOUtil {
void _regularizeRes(crow::response& res, crow::status code,
                    const std::string& body) {
  res.code = code;
  res.body = body;
  res.set_header("Content-Type", "application/json");
  res.end();
}

void return_success(crow::response& res,
                    const crow::json::wvalue& data = nullptr) {
  server::ResultVO rv("00000", "", data);
  _regularizeRes(res, crow::status::OK, rv.dump());
}

void return_error(crow::response& res, const server::Result& result) {
  server::ResultVO rv(result.code, result.msg, nullptr);
  const auto code = result.code;
  auto status = crow::status::INTERNAL_SERVER_ERROR;
  if (code.length() && code[0] == 'C') {
    status = crow::status::BAD_REQUEST;
  } else if (code.length() && code[0] == 'S') {
    status = crow::status::INTERNAL_SERVER_ERROR;
  } else if (code.length() && code[0] == 'U') {
    status = crow::status::OK;
  }
  _regularizeRes(res, status, rv.dump());
}

void return_error(crow::response& res, const std::string& code,
                  const std::string& msg) {
  SPDLOG_INFO("code: {}, msg: {}", code, msg);
  server::ResultVO rv(code, msg, nullptr);
  auto status = crow::status::INTERNAL_SERVER_ERROR;
  if (code.length() && code[0] == 'C') {
    status = crow::status::BAD_REQUEST;
  } else if (code.length() && code[0] == 'S') {
    status = crow::status::INTERNAL_SERVER_ERROR;
  } else if (code.length() && code[0] == 'U') {
    status = crow::status::OK;
  }
  _regularizeRes(res, status, rv.dump());
}
}  // namespace ResultVOUtil
}  // namespace server
}  // namespace mergebot