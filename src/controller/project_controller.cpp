//
// Created by whalien on 09/02/23.
//
#include "mergebot/controller/project_controller.h"

#include <crow/http_response.h>
#include <crow/json.h>
#include <llvm/Support/ErrorOr.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <nlohmann/json.hpp>
#include <unordered_set>
#include <vector>

#include "mergebot/controller/exception_handler_aspect.h"
#include "mergebot/core/ResolutionManager.h"
#include "mergebot/core/model/Project.h"
#include "mergebot/core/sa_utility.h"
#include "mergebot/filesystem.h"
#include "mergebot/server/server_utility.h"
#include "mergebot/utils/fileio.h"
#include "mergebot/utils/gitservice.h"
#include "mergebot/utils/pathop.h"
#include "mergebot/utils/stringop.h"

namespace mergebot {
namespace server {
using json = nlohmann::json;

namespace internal {
/// \brief set merge scenario resolution algorithm running sign in
/// projDir/msName dir. Note that this method should be called in a thread-safe
/// context.
/// \param projDir project cache dir
/// \param msName merge scenario name
void setRunningSign(std::string const& projDir, std::string const& msName) {
  fs::path msPath = fs::path(projDir) / msName;
  // clang-format off
  assert(fs::exists(msPath) &&
             "the merge scenario path should be created before setting running sign");
  // clang-format on
  mergebot::util::file_overwrite_content(msPath / "running", "1");
}

void removeRunningSign(const std::string& msPath) {
  const fs::path runningSign = fs::path(msPath) / "running";
  if (fs::exists(runningSign)) {
    fs::remove(runningSign);
  }
}

bool writeConflictFiles(const fs::path& msCacheDir,
                        const std::string& fileList) {
  const fs::path conflicts = msCacheDir / "conflicts" / "conflict-sources.txt";
  if (!fs::exists(conflicts.parent_path())) {
    fs::create_directories(conflicts.parent_path());
  }
  int fd = open(conflicts.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd == -1) {
    spdlog::error("fail to open file {} for write ,reason: {}",
                  conflicts.c_str(), strerror(errno));
    return false;
  }
  ssize_t bytesWritten = ::write(fd, fileList.c_str(), fileList.length());
  if (bytesWritten == -1) {
    spdlog::error("fail to write to file {},reason: {}", conflicts.c_str(),
                  strerror(errno));
    return false;
  }
  close(fd);
  return true;
}

void goResolve(std::string project, std::string path, sa::MergeScenario& ms,
               [[maybe_unused]] crow::response& res) {
  const std::string cacheDirCheckSum = utils::calcProjChecksum(project, path);
  const fs::path projectCacheDir =
      fs::path(mergebot::util::toabs(MBDIR)) / cacheDirCheckSum;
  const fs::path msCacheDir = projectCacheDir / ms.name;
  /// !!! remember to remove running sign when jump out of control flow
  setRunningSign(projectCacheDir, ms.name);

  // collect conflict files in the project
  std::string command =
      fmt::format("(cd {} && git diff --name-only --diff-filter=U)", path);
  llvm::ErrorOr<std::string> resultOrErr =
      ::mergebot::utils::ExecCommand(command);
  if (!resultOrErr) handleServerExecError(resultOrErr.getError(), command);
  std::string result = resultOrErr.get();

  // get c/cpp related source files
  std::vector<std::string_view> fileNames = util::string_split(result, "\n");
  if (fileNames.size() == 0) {
    removeRunningSign(msCacheDir);
    spdlog::info(
        "there is no conflicting C/C++ sources in the project [{}] located at "
        "[{}]. Currently, we are unable to handle this merge scenario",
        project, path);
    throw AppBaseException(
        "U1000",
        fmt::format(
            "在路径[{}]下的项目[{}]中无冲突的C/C++文件，MBSA暂时无法处理该项目",
            path, project));
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
    removeRunningSign(msCacheDir);
    spdlog::info(
        "current project[{}] has {} conflict files, but none of them are c/cpp "
        "sources, we cannot handle them at this stage",
        project, fileNames.size());
    throw AppBaseException(
        "U1000", fmt::format("当前项目[{}]有{}个冲突文件，但由于都不是C/"
                             "C++相关的源文件，mergebot-sa当前阶段无法处理",
                             project, fileNames.size()));
  }

  spdlog::info(
      "current project[{}, path: {}] has {} conflict files, {} of them are "
      "C/C++ related sources",
      project, path, fileNames.size(), cppSources.size());
  writeConflictFiles(msCacheDir, util::string_join(cppSources.begin(),
                                                   cppSources.end(), "\n"));
  std::unique_ptr<std::string[]> conflictFiles =
      std::make_unique<std::string[]>(cppSources.size());
  std::transform(
      cppSources.begin(), cppSources.end(), conflictFiles.get(),
      [](const std::string_view& fileName) { return std::string(fileName); });

  // construct ResolutionManager, enable shared from this
  std::shared_ptr<sa::ResolutionManager> resolutionManager =
      std::make_shared<sa::ResolutionManager>(
          std::move(project), std::move(path), std::move(ms),
          std::move(conflictFiles), cppSources.size());
  try {
    // call its async doResolution method to do resolution
    resolutionManager->doResolution();
  } catch (const std::exception& ex) {
    removeRunningSign(resolutionManager->mergeScenarioPath());
    spdlog::info("unexpected error caught: {}, unlock merge scenario\n\n\n",
                 ex.what());
    throw ex;
  }
}

bool checkAndAddProjectMetadata(const std::string& project,
                                const std::string& path) {
  const std::string cacheDirCheckSum = utils::calcProjChecksum(project, path);
  const fs::path projectCacheDir =
      fs::path(util::toabs(MBDIR)) / cacheDirCheckSum;
  const fs::path manifestPath =
      fs::path(util::toabs(MBDIR)) /
      fmt::format("manifest-{}.json", cacheDirCheckSum.substr(0, 2));
  sa::Project projToWrite = {
      .project = project, .path = path, .cacheDir = projectCacheDir};

  {
    std::lock_guard<std::mutex> lock(mergebot::utils::peekMutex);
    if (!fs::exists(manifestPath.c_str())) {
      spdlog::info("project manifest file[{}] doesn't exist, we'll create one",
                   manifestPath.c_str());
      fs::create_directories(projectCacheDir);

      std::vector<sa::Project> projList = {projToWrite};
      json projsJson = projList;
      std::string projsJsonStr = projsJson.dump(2);
      const char* buf = projsJsonStr.c_str();

      auto [fd, lck] = mergebot::utils::lockWRFD(manifestPath.c_str());
      if (fd == -1) {
        return false;
      }
      ssize_t bytesWritten = ::write(fd, buf, projsJsonStr.length());
      mergebot::utils::unlockFD(manifestPath.c_str(), fd, lck);
      if (bytesWritten != static_cast<ssize_t>(projsJsonStr.length())) {
        spdlog::error("write to file [{}] failed, reason: {}",
                      manifestPath.c_str(), strerror(errno));
        return false;
      } else {
        return true;
      }
    } else {
      auto [fd, lck] = mergebot::utils::lockRDFD(manifestPath.c_str());
      if (fd == -1) {
        return false;
      }
      FILE* file = fdopen(fd, "r");
      if (!file) {
        spdlog::error("fail to convert file descriptor to FILE*, reason: {}",
                      strerror(errno));
        mergebot::utils::unlockFD(manifestPath.c_str(), fd, lck);
        close(fd);
        return false;
      }
      json projsListJSON = json::parse(file);
      fclose(file);
      std::vector<sa::Project> projList = projsListJSON;
      auto projIt =
          std::find_if(projList.begin(), projList.end(), [&](const auto& proj) {
            return fs::path(proj.cacheDir).filename() == cacheDirCheckSum;
          });
      if (projIt != projList.end()) {
        spdlog::info(
            "project {} already exists in project manifest file[{}], we'll do "
            "nothing",
            path, manifestPath.c_str());
        return true;
      }
      fs::create_directories(projectCacheDir);
      projList.push_back(projToWrite);
      json projsJson = projList;
      std::string projsJsonStr = projsJson.dump(2);
      const char* buf = projsJsonStr.c_str();
      auto pair = mergebot::utils::lockWRFD(manifestPath.c_str());
      fd = pair.first;
      lck = pair.second;
      if (fd == -1) {
        return false;
      }
      ssize_t bytesWritten = ::write(fd, buf, projsJsonStr.length());
      mergebot::utils::unlockFD(manifestPath.c_str(), fd, lck);
      close(fd);
      if (bytesWritten != static_cast<ssize_t>(projsJsonStr.length())) {
        spdlog::error("write to file [{}] failed, reason: {}",
                      manifestPath.c_str(), strerror(errno));
        return false;
      } else {
        return true;
      }
    }
  }
}

bool checkAndAddMSMetadata(const std::string& project, const std::string& path,
                           const sa::MergeScenario& ms) {
  const std::string cacheDirCheckSum = utils::calcProjChecksum(project, path);
  const fs::path projectCacheDir =
      fs::path(util::toabs(MBDIR)) / cacheDirCheckSum;
  const fs::path msCacheDir = projectCacheDir / ms.name;

  const fs::path manifestPath =
      fs::path(util::toabs(MBDIR)) /
      fmt::format("manifest-{}.json", cacheDirCheckSum.substr(0, 2));

  assert(fs::exists(manifestPath) &&
         "project manifest file should exist before write merge scenario "
         "metadata");
  {
    std::lock_guard<std::mutex> lock(mergebot::utils::peekMutex);
    auto [fd, lck] = mergebot::utils::lockRDFD(manifestPath.c_str());
    if (fd == -1) {
      return false;
    }
    FILE* file = fdopen(fd, "r");
    if (!file) {
      spdlog::error("fail to convert file descriptor to FILE*, reason: {}",
                    strerror(errno));
      mergebot::utils::unlockFD(manifestPath.c_str(), fd, lck);
      close(fd);
      return false;
    }
    json projsListJSON = json::parse(file);
    fclose(file);
    std::vector<sa::Project> projsList = projsListJSON;
#ifndef NDEBUG
    auto preWrittenIt =
        std::find_if(projsList.begin(), projsList.end(), [&](const auto& proj) {
          return fs::path(proj.cacheDir).filename() == cacheDirCheckSum;
        });
    assert(preWrittenIt != projsList.end() &&
           "project metadata should exist before add merge scenario metadata");
#endif
    auto projIt =
        std::find_if(projsList.begin(), projsList.end(), [&](const auto& proj) {
          return fs::path(proj.cacheDir).filename() == cacheDirCheckSum;
        });
    auto msIt = std::find_if(
        projIt->mss.begin(), projIt->mss.end(),
        [&](const auto& msItem) { return msItem.name == ms.name; });
    if (msIt != projIt->mss.end()) {
      spdlog::info(
          "merge scenario[{}] metadata is already in project manifest "
          "file[{}], "
          "we'll check if the algorithm is running",
          ms.name, manifestPath.c_str());
      if (fs::exists(msCacheDir / "running")) {
        spdlog::info("the resolution algorithm is running, we'll do nothing");
        throw AppBaseException(
            "C1000",
            fmt::format("合并场景[{}]的冲突解决算法正在运行中...", ms.name));
      } else {
        spdlog::info(
            "the resolution algorithm is not running, we'll start it soon");
        return true;
      }
    } else {
      projIt->mss.push_back(ms);
      fs::create_directories(msCacheDir);
      projsListJSON = projsList;
      std::string content = projsListJSON.dump(2);
      const char* buf = content.c_str();
      auto pair = mergebot::utils::lockWRFD(manifestPath.c_str());
      fd = pair.first;
      lck = pair.second;
      if (fd == -1) {
        return false;
      }
      ssize_t bytesWritten = ::write(fd, buf, content.length());
      mergebot::utils::unlockFD(manifestPath.c_str(), fd, lck);
      close(fd);

      if (bytesWritten != static_cast<ssize_t>(content.length())) {
        spdlog::error("write to file [{}] failed, reason: {}",
                      manifestPath.c_str(), strerror(errno));
        return false;
      } else {
        return true;
      }
    }
  }
}

void handleMergeScenario(const std::string& project, const std::string& path,
                         sa::MergeScenario& ms, crow::response& res) {
  const std::string cacheDirCheckSum = utils::calcProjChecksum(project, path);
  const fs::path manifestPath =
      fs::path(util::toabs(MBDIR)) /
      fmt::format("manifest-{}.json", cacheDirCheckSum.substr(0, 2));

  bool Success = checkAndAddProjectMetadata(project, path);
  if (!Success) {
    spdlog::error(
        "fail to add project metadata[{}] to project manifest file[{}]", path,
        manifestPath.c_str());
  } else {
    Success = checkAndAddMSMetadata(project, path, ms);
    if (!Success) {
      spdlog::error(
          "fail to add merge scenario metadata[{}] to project manifest "
          "file[{}]",
          ms, manifestPath.c_str());
    }
  }

  goResolve(project, path, ms, res);
}

crow::json::wvalue doPostMergeScenario(const crow::request& req,
                                       crow::response& res) {
  const auto body = crow::json::load(req.body);
  // project name is optional: if absent, fill with basename of the project path
  if (body.error() || !utils::containKeys(body, {"path", "ms"}) ||
      !utils::containKeys(body["ms"], {"ours", "theirs"})) {
    spdlog::error("the format of request body data is illegal");
    throw AppBaseException(ResultEnum::BAD_REQUEST);
  }

  const auto path = static_cast<std::string>(body["path"]);
  const auto project = body.has("project")
                           ? static_cast<std::string>(body["project"])
                           : fs::path(path).filename().string();
  utils::checkPath(path);
  utils::checkGitRepo(path);

  std::string ours = utils::validateAndCompleteRevision(
      static_cast<std::string>(body["ms"]["ours"]), path);
  std::string theirs = utils::validateAndCompleteRevision(
      static_cast<std::string>(body["ms"]["theirs"]), path);
  auto baseOpt = util::git_merge_base(ours, theirs, path);
  std::string base = baseOpt.has_value() ? baseOpt.value() : "";

  sa::MergeScenario ms(ours, theirs, base);
  internal::handleMergeScenario(project, path, ms, res);
  // default construct a crow::json::wvalue to indicate return successfully
  return {};
}
}  // namespace internal

void PostMergeScenario(const crow::request& req, crow::response& res) {
  auto internalPostMergeScenario = ExceptionHandlerAspect<CReqMResFuncType>(
      internal::doPostMergeScenario, res);
  auto rv = internalPostMergeScenario(req, res);
  if (!err(rv)) ResultVOUtil::return_success(res, rv);
}
}  // namespace server
}  // namespace mergebot
