//
// Created by whalien on 04/03/23.
//

#ifndef MB_RESOLUTIONMANAGER_H
#define MB_RESOLUTIONMANAGER_H

#include "HandlerChain.h"
#include "mergebot/core/model/MergeScenario.h"
#include "mergebot/filesystem.h"
#include "mergebot/globals.h"
#include "mergebot/utils/pathop.h"
#include "mergebot/utils/sha1.h"

#include <llvm/ADT/StringSet.h>
#include <llvm/Support/Error.h>
#include <memory>
#include <string>

namespace mergebot {
namespace sa {

class ResolutionManager
    : public std::enable_shared_from_this<ResolutionManager> {
public:
  friend class HandlerChain;
  ResolutionManager(std::string &&Project_, std::string &&ProjectPath_,
                    sa::MergeScenario &&MS_,
                    std::unique_ptr<std::string[]> &&ConflictFiles_,
                    int FileNum)
      : Project_(std::move(Project_)), MS_(std::move(MS_)),
        ConflictFiles_(std::move(ConflictFiles_)), CurrIdx_(0),
        FileNum_(FileNum) {
    if (!ProjectPath_.empty() && ProjectPath_[ProjectPath_.size() - 1] !=
                                     fs::path::preferred_separator) {
      ProjectPath_ += fs::path::preferred_separator;
    }
    this->ProjectPath_ = std::move(ProjectPath_);
  }

  /// get current project's cache dir, in the following form:
  /// /home/whalien/.mergebot/f690cc3bcbb89d40a7e484ff47e38baad3f55a5a
  /// \return merge scenario cache dir, no trailing slash
  std::string projectCacheDir() const {
    const fs::path HomePath = fs::path(mergebot::util::toabs(MBDIR));
    return HomePath / projectCheckSum();
  }

  /// get current merge scenario of current project's cache dir, in the
  /// following form:
  /// /home/whalien/.mergebot/f690cc3bcbb89d40a7e484ff47e38baad3f55a5a/b9b352-fa24d4
  /// \return merge scenario cache dir
  std::string mergeScenarioPath() const {
    const fs::path homePath = fs::path(mergebot::util::toabs(MBDIR));
    return homePath / projectCheckSum() / MS_.name;
  }

  /// return project check sum of current project in the following form:
  /// f690cc3bcbb89d40a7e484ff47e38baad3f55a5a
  /// \return project checksum string
  std::string projectCheckSum() const {
    util::SHA1 checksum;
    std::string Path = ProjectPath_;
    if (Path.back() == fs::path::preferred_separator) {
      Path.pop_back();
    }
    checksum.update(fmt::format("{}-{}", Project_, Path));
    return checksum.final();
  }

  /// project cache dir
  /// \return project cache dir which  has a trailing system specific path
  /// separator
  std::string_view projectPath() const noexcept { return ProjectPath_; }
  /// project name
  /// \return project name
  std::string_view project() const noexcept { return Project_; }
  /// call static_cast<std::string>(MS_) to get an identification of Merge
  /// Scenario \return string representation of MergeScenario
  std::string mergeScenario() const noexcept {
    return static_cast<std::string>(MS_);
  };

  void doResolution();

private:
  const static std::string CompDBRelative;

  static bool fineTuneCompDB(const std::string &CompDBPath,
                             const std::string& ProjPath,
                             const std::string &OrigPath);
  static void
  _doResolutionAsync(std::shared_ptr<ResolutionManager> const &Self);
  static void prepareSource(std::shared_ptr<ResolutionManager> const &Self,
                            std::string const &CommitHash,
                            std::string const &SourceDest);

  std::vector<std::string> _extractCppSources();

  // basic information
  std::string Project_;     // project name, such as frameworks_av
  std::string ProjectPath_; // original project path, like
                            // /home/whalien/Desktop/frameworks_av
  MergeScenario MS_;        // Merge Scenario we're handling

  // conflict files list, use [`ConflictFiles_.get()`, `ConflictFiles_.get() +
  // FileNum_`) to iterate over the files
  std::unique_ptr<std::string[]> ConflictFiles_;
  int CurrIdx_;
  int FileNum_;

  // resolved files set and mutex. Do we really need it?
  llvm::StringSet<> ResolvedFiles_;
  std::mutex ResolvedMutex_;
};

} // namespace sa
} // namespace mergebot

#endif // MB_RESOLUTIONMANAGER_H
