//
// Created by whalien on 09/02/23.
//

#include "mergebot/controller/resolve_controller.h"

#include <spdlog/spdlog.h>

#include "mergebot/controller/exception_handler_aspect.h"
#include "mergebot/core/sa_utility.h"
#include "mergebot/filesystem.h"
#include "mergebot/globals.h"
#include "mergebot/utils/gitservice.h"
#include "mergebot/utils/pathop.h"

namespace mergebot {
namespace server {
using json = nlohmann::json;
namespace detail {
void normalizeRelativePath(std::string& path, const std::string& replacement) {
  if (util::starts_with(path, replacement)) {
    path.replace(0, replacement.length(), "");
  }
}
}  // namespace detail

namespace internal {
crow::json::wvalue getFileResolution(const std::string& project,
                                     const std::string& path,
                                     const sa::MergeScenario& ms,
                                     const std::string& file,
                                     crow::response& res) {
  crow::json::wvalue data;
  const std::string cacheDirCheckSum = utils::calcProjChecksum(project, path);
  const fs::path projectCacheDir =
      fs::path(mergebot::util::toabs(MBDIR)) / cacheDirCheckSum;
  const fs::path msCacheDir = projectCacheDir / ms.name;

  data["pending"] = fs::exists(msCacheDir / "running");
  data["projectPath"] = path;

  std::string fileNormalized = file;
  detail::normalizeRelativePath(fileNormalized, "./");
  detail::normalizeRelativePath(fileNormalized, ".\\");
  if (fs::path(file).is_absolute()) {
    fileNormalized = fs::relative(fs::path(file), path);
  }
  data["file"] = fileNormalized;

  std::vector<crow::json::wvalue> resolutionList;
  std::string fileName = sa::pathToName(fileNormalized);
  const fs::path resolutionFile = msCacheDir / "resolutions" / fileName;
  if (fs::exists(resolutionFile)) {
    auto [fd, lck] = mergebot::utils::lockRDFD(resolutionFile.c_str());
    if (fd == -1) {
      spdlog::error("fail to open file [{}] to read resolution results",
                    resolutionFile.c_str());
    }
    FILE* file_ptr = fdopen(fd, "r");
    if (!file_ptr) {
      spdlog::error("fail to convert fd to FILE* object, reason: {}",
                    strerror(errno));
      mergebot::utils::unlockFD(resolutionFile.c_str(), fd, lck);
      fclose(file_ptr);
    } else {
      json frrJSON = json::parse(file_ptr);
      FileResolutionResult frr = frrJSON;

      auto string_spilt = [&](const std::string& str, const std::string& delims,
                              bool with_empty = false) {
        std::vector<std::string> output;

        for (auto first = str.data(), second = str.data(),
                  last = first + str.size();
             second != last && first != last; first = second + 1) {
          second = std::find_first_of(first, last, std::cbegin(delims),
                                      std::cend(delims));

          if (first != second || with_empty) {
            output.emplace_back(first, second - first);
          }
        }
        return output;
      };
      std::for_each(frr.resolutions.begin(), frr.resolutions.end(),
                    [&](const auto& block) {
                      crow::json::wvalue resBlock;
                      resBlock["code"] = string_spilt(block.code, "\n", true);
                      resBlock["index"] = block.index - 1;
                      resBlock["label"] = "";
                      resBlock["desc"] = block.desc;
                      resBlock["confidence"] = 0.7;
                      resolutionList.push_back(std::move(resBlock));
                    });
      mergebot::utils::unlockFD(resolutionFile.c_str(), fd, lck);
      fclose(file_ptr);
    }
  }

  data["resolutions"] = std::move(resolutionList);

  return data;
}

crow::json::wvalue doGetFileResolution(const crow::request& req,
                                       crow::response& res) {
  const auto body = crow::json::load(req.body);
  // project name is optional: if absent, fill with basename of the project
  // path
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

  return internal::getFileResolution(project, path, ms, file, res);
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
