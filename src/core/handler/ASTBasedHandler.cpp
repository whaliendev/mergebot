//
// Created by whalien on 13/03/23.
//

#include "mergebot/core/handler/ASTBasedHandler.h"
#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/filesystem.h"
#include "mergebot/utils/fileio.h"

#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/JSONCompilationDatabase.h>
#include <filesystem>
#include <fstream>
#include <llvm/ADT/StringRef.h>
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
  if (!fs::exists(OurCompDB) || !fs::exists(TheirCompDB) ||
      !fs::exists(BaseCompDB)) {
    spdlog::warn("CompDB doesn't exist, we'll skip AST based handler");
    return;
  }
  replaceProjPath(OurCompDB, OurDir);
  replaceProjPath(TheirCompDB, TheirDir);
  replaceProjPath(BaseCompDB, BaseDir);

  //  bool ok1 = OurCompilationsPair.second;
  auto &[OurCompilations, ok1] = OurCompilationsPair;
  auto &[BaseCompilations, ok2] = BaseCompilationsPair;
  auto &[TheirCompilations, ok3] = TheirCompilationsPair;
  if (!ok1 || !ok2 || !ok3) {
    spdlog::warn("cannot read CompDB, we'll skip AST based analysis");
    return;
  }

  std::vector<std::string> OurConflictFiles =
      collectAnalysisFileSet(ConflictFiles, OurDir);
  std::vector<std::string> BaseConflictFiles =
      collectAnalysisFileSet(ConflictFiles, BaseDir);
  std::vector<std::string> TheirConflictFiles =
      collectAnalysisFileSet(ConflictFiles, TheirDir);
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

  // remove trailing separator if it has
  std::string ProjectPath = Meta.ProjectPath;
  if (ProjectPath.back() == fs::path::preferred_separator) {
    ProjectPath.pop_back();
  }
  std::string FileData = ss.str();
  re2::RE2 OriginalPath("(" + re2::RE2::QuoteMeta(ProjectPath) + ")");
  assert(OriginalPath.ok() && "fail to compile regex for CompDB");
  int ReplaceCnt = re2::RE2::GlobalReplace(&FileData, OriginalPath, ProjPath);
#ifndef NDEBUG
  if (!ReplaceCnt) {
    spdlog::warn("we replaced 0 original project path, which is weird");
  }
#endif

  util::file_overwrite_content(CompDBPath, FileData);
  return true;
}

void ASTBasedHandler::initCompDB() {
  std::string ErrMsg;
  OurCompilationsPair.first = clang::tooling::inferMissingCompileCommands(
      clang::tooling::JSONCompilationDatabase::loadFromFile(
          OurCompDB, ErrMsg,
          clang::tooling::JSONCommandLineSyntax::AutoDetect));
  //));
  OurCompilationsPair.second = true;
  if (!ErrMsg.empty()) {
    spdlog::error(ErrMsg, OurCompDB);
    OurCompilationsPair.second = false;
  }

  ErrMsg.clear();
  BaseCompilationsPair.first = clang::tooling::inferMissingCompileCommands(
      clang::tooling::JSONCompilationDatabase::loadFromFile(
          BaseCompDB, ErrMsg,
          clang::tooling::JSONCommandLineSyntax::AutoDetect));
  //)
  BaseCompilationsPair.second = true;
  if (!ErrMsg.empty()) {
    spdlog::error(ErrMsg, BaseCompDB);
    BaseCompilationsPair.second = false;
  }

  ErrMsg.clear();
  TheirCompilationsPair.first = clang::tooling::inferMissingCompileCommands(
      clang::tooling::JSONCompilationDatabase::loadFromFile(
          OurCompDB, ErrMsg,
          clang::tooling::JSONCommandLineSyntax::AutoDetect));
  //)
  TheirCompilationsPair.second = true;
  if (!ErrMsg.empty()) {
    spdlog::error(ErrMsg, TheirCompDB);
    TheirCompilationsPair.second = false;
  }
}

} // namespace sa
} // namespace mergebot
