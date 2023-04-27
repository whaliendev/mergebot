//
// Created by whalien on 13/03/23.
//

#include "mergebot/core/handler/ASTBasedHandler.h"
#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/filesystem.h"
#include "mergebot/utils/fileio.h"

#include <clang/Frontend/ASTUnit.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/JSONCompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/StringRef.h>
#include <memory>
#include <re2/re2.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <system_error>
#include <unordered_set>
#include <vector>

namespace mergebot {
namespace sa {

const std::string ASTBasedHandler::CompDBRelative =
    "build/compile_commands.json";

void ASTBasedHandler::resolveConflictFiles(
    std::vector<ConflictFile> &ConflictFiles) {
  assert(ConflictFiles.size() &&
         "ConflictFile sizes should be greater than zero");

  spdlog::info("we are resolving conflicts using AST based handler");
  if (!fs::exists(OurCompDB) || !fs::exists(TheirCompDB) ||
      !fs::exists(BaseCompDB)) {
    spdlog::warn("CompDB doesn't exist, we'll skip AST based handler");
    return;
  }

  initCompDB();

  replaceProjPath(OurCompDB, OurDir);
  replaceProjPath(TheirCompDB, TheirDir);
  replaceProjPath(BaseCompDB, BaseDir);

  auto &[OurCompilations, ok1] = OurCompilationsPair;
  auto &[BaseCompilations, ok2] = BaseCompilationsPair;
  auto &[TheirCompilations, ok3] = TheirCompilationsPair;
  if (!ok1 || !ok2 || !ok3) {
    spdlog::warn("cannot read CompDB, we'll skip AST based analysis");
    return;
  }

  std::unordered_set<std::string> OurConflictFiles =
      collectAnalysisFileSet(ConflictFiles, OurDir, OurCompilations);
  std::unordered_set<std::string> BaseConflictFiles =
      collectAnalysisFileSet(ConflictFiles, BaseDir, BaseCompilations);
  std::unordered_set<std::string> TheirConflictFiles =
      collectAnalysisFileSet(ConflictFiles, TheirDir, TheirCompilations);
}

std::unordered_set<std::string> ASTBasedHandler::collectAnalysisFileSet(
    std::vector<ConflictFile> const &ConflictFiles,
    std::string_view ProjectPath,
    const std::unique_ptr<clang::tooling::CompilationDatabase> &Compilations)
    const {
  std::unordered_set<std::string> AnalysisFileSet;

  std::vector<std::string> SourcePaths;
  SourcePaths.reserve(ConflictFiles.size());
  for (const auto &CF : ConflictFiles) {
    SourcePaths.push_back(fs::path(ProjectPath) /
                          fs::relative(CF.Filename, Meta.ProjectPath));
  }

  clang::tooling::ClangTool Tool(*Compilations, SourcePaths);
  std::vector<std::unique_ptr<clang::ASTUnit>> TUs;
  Tool.buildASTs(TUs);
  for (const std::unique_ptr<clang::ASTUnit> &TU : TUs) {
    clang::Preprocessor &PP = TU->getPreprocessor();
    clang::Preprocessor::IncludedFilesSet &IncludedFiles =
        PP.getIncludedFiles();
    for (const auto *FileEntry : IncludedFiles) {
      if (fs::path(FileEntry->getName().str()).is_relative()) {
        std::error_code EC;
        fs::path FullPath = fs::canonical(
            fs::path(ProjectPath) / FileEntry->getName().str(), EC);
        if (!EC) {
          AnalysisFileSet.insert(FullPath);
        }
      }
    }
  }
  spdlog::debug("size: {}, fileset: {}", AnalysisFileSet.size(),
                fmt::join(AnalysisFileSet, ", "));
  return AnalysisFileSet;
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
  TheirCompilationsPair.second = true;
  if (!ErrMsg.empty()) {
    spdlog::error(ErrMsg, TheirCompDB);
    TheirCompilationsPair.second = false;
  }
}

} // namespace sa
} // namespace mergebot
