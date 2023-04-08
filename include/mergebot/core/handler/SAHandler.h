//
// Created by whalien on 13/03/23.
//

#ifndef MB_SAHANDLER_H
#define MB_SAHANDLER_H

#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/core/model/MergeScenario.h"

#include <string>
#include <vector>

#include <spdlog/spdlog.h>

namespace mergebot {
namespace sa {
struct ProjectMeta {
  std::string Project;
  std::string ProjectPath;
  std::string ProjectCacheDir;
  MergeScenario MS;
  std::string MSCacheDir;
};

/// virtual base handler in HandlerChain, class extends it should declare a ctor
/// which accepts a `ProjectMeta` in copy manner and Handler's name. As
/// SAHandler's ctor will take the ownership of ProjectMeta
class SAHandler {
public:
  explicit SAHandler(ProjectMeta Meta, std::string Name = __FILE_NAME__)
      : Meta(std::move(Meta)), Skip_(false), Name_(Name),
        NextHandler_(nullptr) {}
  virtual ~SAHandler() {}

  void handle(std::vector<ConflictFile> &ConflictFiles) const {
    if (Skip_ && NextHandler_) {
      spdlog::info("skip sa handler {} to next handler {}", Name_,
                   NextHandler_->name());
    } else if (!Skip_) {
      resolveConflictFiles(ConflictFiles);
      if (ConflictFiles.size() && NextHandler_) {
        NextHandler_->handle(ConflictFiles);
      } else if (ConflictFiles.size() && !NextHandler_) {
        spdlog::info("in project {}, we have reached the final sa handler. "
                     "However, there are still some conflicts. ",
                     Meta.Project);
        reportResolutionResult(ConflictFiles);
      } else {
        spdlog::info("Incredible! All conflicts are resolved");
      }
    } else {
      spdlog::info("skip sa handler {} to next handler. However, we reached "
                   "the end of handler chain");
      if (ConflictFiles.size() > 0) {
        reportResolutionResult(ConflictFiles);
      } else {
        spdlog::info("Incredible! All the conflicts are resolved");
      }
    }
  }

  void setNext(SAHandler *NextHandler) noexcept { NextHandler_ = NextHandler; }

  std::string_view name() const noexcept { return Name_; }
  void setSkip() noexcept { Skip_ = true; }
  void clearSkip() noexcept { Skip_ = false; }

protected:
  ProjectMeta Meta;

  void reportResolutionResult(std::vector<ConflictFile> &ConflictFiles) const {
    spdlog::info("there are still {} conflict files in project {}: ",
                 ConflictFiles.size(), Meta.Project);

    std::vector<std::string_view> Filenames;
    std::for_each(
        ConflictFiles.begin(), ConflictFiles.end(),
        [&](const ConflictFile &CF) { Filenames.push_back(CF.Filename); });
    // clang-format off
    spdlog::info(fmt::format(R"(they are respectively [
        {}
    ])", fmt::join(Filenames, ",\n\t")));
    // clang-format on
  }

private:
  virtual void
  resolveConflictFiles(std::vector<ConflictFile> &ConflictFiles) const = 0;

  bool Skip_;
  std::string Name_;
  SAHandler *NextHandler_;
};
} // namespace sa
} // namespace mergebot

#endif // MB_SAHANDLER_H
