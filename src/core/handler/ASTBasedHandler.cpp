//
// Created by whalien on 13/03/23.
//

#include "mergebot/core/handler/ASTBasedHandler.h"
#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/filesystem.h"
#include "mergebot/utility.h"
#include "mergebot/utils/fileio.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/MemoryBuffer.h>
#include <memory>
#include <re2/re2.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <system_error>
#include <vector>

namespace mergebot {
namespace sa {

const std::string ASTBasedHandler::CompDBRelative =
    "build/compile_commands.json";

void ASTBasedHandler::resolveConflictFiles(
    std::vector<ConflictFile> &ConflictFiles) const {
  assert(ConflictFiles.size() &&
         "ConflictFile sizes should be greater than zero");

  spdlog::info("we are resolving conflicts using AST based handler");

  const fs::path OurCompDB =
      fs::path(Meta.MSCacheDir) / "ours" / CompDBRelative;
  const fs::path TheirCompDB =
      fs::path(Meta.MSCacheDir) / "theirs" / CompDBRelative;
  const fs::path BaseCompDB =
      fs::path(Meta.MSCacheDir) / "base" / CompDBRelative;
  if (!fs::exists(OurCompDB) || !fs::exists(TheirCompDB) ||
      !fs::exists(BaseCompDB)) {
    spdlog::warn(
        "compile commands not provided, we'll skip AST based analysis");
    return;
  }

  std::string OurPath = fs::path(Meta.MSCacheDir) / "ours";
  std::string TheirPath = fs::path(Meta.MSCacheDir) / "theirs";
  std::string BasePath = fs::path(Meta.MSCacheDir) / "base";

  replaceProjPath(OurCompDB, OurPath);
  replaceProjPath(TheirCompDB, TheirPath);
  replaceProjPath(BaseCompDB, BasePath);

  std::vector<std::string> OurConflicts =
      collectAnalysisFileSet(ConflictFiles, OurPath);
}

std::vector<std::string> ASTBasedHandler::collectAnalysisFileSet(
    std::vector<ConflictFile> const &ConflictFiles,
    std::string_view ProjectPath) const {

  return std::vector<std::string>();
}

bool ASTBasedHandler::replaceProjPath(std::string const &CompDBPath,
                                      std::string_view ProjPath) const {
  std::ifstream CompDB(CompDBPath);
  if (!CompDB.is_open()) {
    spdlog::error("failed to open CompDB {}", CompDBPath);
    return false;
  }

  std::stringstream ss;
  std::streambuf *FileBuf = CompDB.rdbuf();
  char Buffer[1024];
  std::size_t BytesRead;
  while ((BytesRead = FileBuf->sgetn(Buffer, sizeof(Buffer))) > 0) {
    ss.write(Buffer, BytesRead);
  }

  std::string ProjectPath = Meta.ProjectPath;
  if (ProjectPath.back() == fs::path::preferred_separator) {
    ProjectPath.pop_back();
  }
  std::string FileData = ss.str();
  re2::RE2 OldPath("(" + re2::RE2::QuoteMeta(ProjectPath) + ")");
  assert(OldPath.ok() && "fail to compile regex for CompDB");
  int ReplaceCnt = re2::RE2::GlobalReplace(&FileData, OldPath, ProjPath);
#ifndef NDEBUG
  if (!ReplaceCnt) {
    spdlog::warn("we replaced 0 original project path, which is weird");
  }
#endif

  util::file_overwrite_content(CompDBPath, FileData);

  return true;
}

} // namespace sa
} // namespace mergebot
