//
// Created by whalien on 09/02/23.
//

#include "mergebot/controller/resolve_controller.h"

#include <spdlog/spdlog.h>

#include "mergebot/controller/exception_handler_aspect.h"
#include "mergebot/filesystem.h"
#include "mergebot/utils/gitservice.h"

namespace mergebot {
namespace server {
namespace detail {}

namespace internal {
void getFileResolution(const std::string& project, const std::string& path,
                       const sa::MergeScenario& ms, const std::string& file,
                       crow::response& res) {}

crow::json::wvalue doGetFileResolution(const crow::request& req,
                                       crow::response& res) {
  const auto body = crow::json::load(req.body);
  // project name is optional: if absent, fill with basename of the project path
  if (body.error() || !utils::containKeys(body, {"path", "file", "ms"}) ||
      !utils::containKeys(body["ms"], {"ours", "theirs"})) {
    spdlog::error("the format of request body data is illegal");
    throw AppBaseException(ResultEnum::BAD_REQUEST);
  }

  const auto path = static_cast<std::string>(body["path"]);
  const auto file = static_cast<std::string>(body["file"]);
  const auto project = body.has("project")
                           ? static_cast<std::string>(body["project"])
                           : fs::path(path).filename().string();
  utils::checkPath(path);
  utils::checkGitRepo(path);

  std::string ours = static_cast<std::string>(body["ms"]["ours"]);
  std::string theirs = static_cast<std::string>(body["ms"]["theirs"]);
  auto baseOpt = util::git_merge_base(ours, theirs, path);
  std::string base = baseOpt.has_value() ? baseOpt.value() : "";
  sa::MergeScenario ms(ours, theirs, base);
  utils::validateAndCompleteCommitHash(ms, path);
  bool success = utils::checkMSMetadata(project, path, ms);
  if (!success) {
    throw AppBaseException(
        "S1000",
        fmt::format(
            "服务端异常：未能成功打开项目manifest文件，请查看服务端日志"));
  }
  utils::checkConflictFile(project, path, ms, file);

  internal::getFileResolution(project, path, ms, file, res);
  // default construct a crow::json::wvalue to indicate return successfully
  return {};
}
}  // namespace internal

void GetFileResolution(const crow::request& req, crow::response& res) {
  auto internalGetFileResolution = ExceptionHandlerAspect<CReqMResFuncType>(
      internal::doGetFileResolution, res);
  auto rv = internalGetFileResolution(req, res);
  if (!err(rv)) ResultVOUtil::return_success(res, rv);
}
}  // namespace server
}  // namespace mergebot
