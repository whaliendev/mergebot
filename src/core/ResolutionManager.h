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

#include <llvm/ADT/StringSet.h>
#include <llvm/Support/Error.h>
#include <memory>
#include <string>

namespace mergebot {
namespace sa {

class ResolutionManager
    : public std::enable_shared_from_this<ResolutionManager> {
public:
  ResolutionManager(std::string &&Project_, std::string &&ProjectPath_,
                    sa::MergeScenario &&MS_,
                    std::unique_ptr<std::string[]> &&ConflictFiles_,
                    int FileNum)
      : Project_(std::move(Project_)), ProjectPath_(std::move(ProjectPath_)),
        MS_(std::move(MS_)), ConflictFiles_(std::move(ConflictFiles_)),
        FileNum_(FileNum), CurrIdx_(0), CompDBCount_(0) {}

  std::string mergeScenarioPath() const {
    const fs::path homePath = fs::path(mergebot::util::toabs(MBDIR));
    return homePath / projectCheckSum() / MS_.name;
  }
  std::string projectCheckSum() const {
    util::SHA1 checksum;
    checksum.update(fmt::format("{}-{}", Project_, ProjectPath_));
    return checksum.final();
  }
  std::string_view projectPath() const noexcept { return ProjectPath_; }
  std::string_view project() const noexcept { return Project_; }
  std::string mergeScenario() const noexcept {
    return static_cast<std::string>(MS_);
  };

  void doResolution();

private:
  static void
  _doResolutionAsync(std::shared_ptr<ResolutionManager> const &Self);

  static void _generateCompDB(std::shared_ptr<ResolutionManager> const &Self,
                              std::string const &CommitHash,
                              std::string const &SourceDest);

  // basic information
  std::string Project_;
  std::string ProjectPath_;
  MergeScenario MS_;

  // conflict files list, use [`ConflictFiles_.get()`, `ConflictFiles_.get() +
  // FileNum_`) to iterate over the files
  std::unique_ptr<std::string[]> ConflictFiles_;
  int CurrIdx_;
  int FileNum_;

  // resolved files set and mutex. Do we really need it?
  llvm::StringSet<> ResolvedFiles_;
  std::mutex ResolvedMutex_;

  // CompDB generation related
  std::mutex CompDBMtx_;
  std::condition_variable CompDBCV_;
  int CompDBCount_;
};

} // namespace sa
} // namespace mergebot

#endif // MB_RESOLUTIONMANAGER_H
