//
// Created by whalien on 09/02/23.
//
#include "project_controller.h"

#include <crow/http_response.h>
#include <crow/json.h>
#include <spdlog/spdlog.h>

#include <vector>

#include "../server/utility.h"
#include "exception_handler_aspect.h"
#include "mergebot/filesystem.h"
#include "mergebot/utils/format.h"

namespace mergebot {
namespace server {
namespace internal {
void __checkPath(std::string const& pathStr) {
  namespace fs = std::filesystem;
  const fs::path dirPath = pathStr;
  const auto rwPerms = static_cast<int>(
      (fs::status(dirPath).permissions() & (fs::perms::owner_read | fs::perms::owner_write)));
  if (fs::exists(dirPath)) {
    if (!fs::is_directory(dirPath) || !rwPerms) {
      spdlog::warn(format("no permission to access path [{}]", pathStr));
      throw AppBaseException("U1000", format("no permission to access path [{}]", pathStr));
    }
  } else {
    spdlog::warn(format("project path [{}] doesn't exist", pathStr));
    throw AppBaseException("U1000", format("project path [{}] doesn't exist", pathStr));
  }
}

void __checkGitDir(std::string const& path) {
  namespace fs = std::filesystem;
  const fs::path gitDirPath = fs::path(path) / ".git";
  if (!(fs::exists(gitDirPath) && fs::is_directory(gitDirPath))) {
    spdlog::warn(format("project path[{}] is not a valid git repo", path));
    throw AppBaseException("U1000", format("project path[{}] is not a valid git repo", path));
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

void __initProject(std::string const& project, std::string const& path) {}

crow::json::wvalue __doPostProject(const crow::request& req, crow::response& res) {
  // 检查目录是否有效，检查是否为git仓库，检查是否为初次运行，生成唯一目录，映射文件
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
