//
// Created by whalien on 04/03/23.
//

#ifndef MB_RESOLUTIONMANAGER_H
#define MB_RESOLUTIONMANAGER_H

#include "../globals.h"
#include "mergebot/filesystem.h"
#include "mergebot/utils/pathop.h"
#include "mergebot/utils/sha1.h"
#include "model/MergeScenario.h"

#include <llvm/Support/Error.h>
#include <string>

namespace mergebot {
namespace sa {

class ResolutionManager {
public:
  ResolutionManager(std::string &&Project_, std::string &&ProjectPath_,
                    sa::MergeScenario &&MS_,
                    std::unique_ptr<std::string[]> &&ConflictFiles_,
                    int FileNum)
      : Project_(std::move(Project_)), ProjectPath_(std::move(ProjectPath_)),
        MS_(std::move(MS_)), ConflictFiles_(std::move(ConflictFiles_)),
        FileNum_(FileNum) {}
  std::string mergeScenarioPath() const {
    const fs::path homePath = fs::path(mergebot::util::toabs(MBDIR));
    return homePath / projectCheckSum() / MS_.name;
  }
  std::string projectCheckSum() const {
    util::SHA1 checksum;
    checksum.update(fmt::format("{}-{}", Project_, ProjectPath_));
    return checksum.final();
  }
  std::string_view projectPath() const { return ProjectPath_; }
  std::string_view project() const { return Project_; }
  std::string mergeScenario() const { return static_cast<std::string>(MS_); };
  void doResolution();

private:
  std::string Project_;
  std::string ProjectPath_;
  MergeScenario MS_;
  std::unique_ptr<std::string[]> ConflictFiles_;
  int FileNum_;
};

} // namespace sa
} // namespace mergebot

#endif // MB_RESOLUTIONMANAGER_H
