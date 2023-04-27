//
// Created by whalien on 13/03/23.
//

#ifndef MB_ASTBASEDHANDLER_H
#define MB_ASTBASEDHANDLER_H

#include "SAHandler.h"

#include <clang/Tooling/CompilationDatabase.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_set>
#include <vector>

#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/filesystem.h"

namespace mergebot {
namespace sa {
class ASTBasedHandler : public SAHandler {
public:
  explicit ASTBasedHandler(ProjectMeta Meta, std::string Name = __FILE_NAME__)
      : SAHandler(Meta, Name) {
    OurCompDB = fs::path(Meta.MSCacheDir) / "ours" / CompDBRelative;
    TheirCompDB = fs::path(Meta.MSCacheDir) / "theirs" / CompDBRelative;
    BaseCompDB = fs::path(Meta.MSCacheDir) / "base" / CompDBRelative;

    OurDir = fs::path(Meta.MSCacheDir) / "ours";
    TheirDir = fs::path(Meta.MSCacheDir) / "theirs";
    BaseDir = fs::path(Meta.MSCacheDir) / "base";
  }

private:
  void resolveConflictFiles(std::vector<ConflictFile> &ConflictFiles) override;

  std::unordered_set<std::string> collectAnalysisFileSet(
      const std::vector<ConflictFile> &ConflictFiles,
      std::string_view ProjectPath,
      const std::unique_ptr<clang::tooling::CompilationDatabase> &CompDB) const;

  bool replaceProjPath(std::string const &CompDBPath,
                       std::string_view ProjPath) const;

  void initCompDB();

  const static std::string CompDBRelative;

  // CompDB file
  std::string OurCompDB;
  std::string BaseCompDB;
  std::string TheirCompDB;

  // three revisions source dir
  std::string OurDir;
  std::string BaseDir;
  std::string TheirDir;

  std::pair<std::unique_ptr<clang::tooling::CompilationDatabase>, bool>
      OurCompilationsPair;
  std::pair<std::unique_ptr<clang::tooling::CompilationDatabase>, bool>
      BaseCompilationsPair;
  std::pair<std::unique_ptr<clang::tooling::CompilationDatabase>, bool>
      TheirCompilationsPair;
};
} // namespace sa
} // namespace mergebot

#endif // MB_ASTBASEDHANDLER_H
