//
// Created by whalien on 13/03/23.
//

#include "mergebot/core/handler/ASTBasedHandler.h"
#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/core/semantic/GraphBuilder.h"
#include "mergebot/core/semantic/SourceCollectorV2.h"
#include "mergebot/filesystem.h"
#include "mergebot/utils/fileio.h"
#include "mergebot/utils/stringop.h"
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/JSONCompilationDatabase.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <llvm/ADT/StringRef.h>
#include <oneapi/tbb/task_group.h>
#include <oneapi/tbb/tick_count.h>
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
    std::vector<ConflictFile> &ConflictFiles) {
  assert(ConflictFiles.size() &&
         "ConflictFile sizes should be greater than zero");

  spdlog::info("we are resolving conflicts using AST based handler...");
  spdlog::info("dependencies analysis disabled due to lack of CompDB");

  /// init CompDB
  spdlog::info("we are resolving conflicts using AST based handler");
  if (!fs::exists(OurCompDB) || !fs::exists(TheirCompDB) ||
      !fs::exists(BaseCompDB)) {
    spdlog::warn("CompDB doesn't exist, we'll skip AST based handler");
    return;
  }

  initCompDB();
  auto &[OurCompilations, ok1] = OurCompilationsPair;
  auto &[BaseCompilations, ok2] = BaseCompilationsPair;
  auto &[TheirCompilations, ok3] = TheirCompilationsPair;
  if (!ok1 || !ok2 || !ok3) {
    spdlog::warn("cannot read CompDB, we'll skip AST based analysis");
    return;
  }

  /// do variant intelli merge
  // 1. Collect Sources set to analyze
  // note that LookupIncluded is default enabled, OnlyBothModified is default
  // disabled
  SourceCollectorV2 SC(Meta, OurCompilations, BaseCompilations,
                       TheirCompilations);
  SC.collectAnalysisSources();
  SourceCollectorV2::AnalysisSourceTuple ST = SC.analysisSourceTuple();

  // 2. Get Graph representation of 3 commit nodes
  std::vector<std::string> ConflictPaths;
  ConflictPaths.reserve(ConflictFiles.size());
  for (ConflictFile const &CF : ConflictFiles) {
    std::string RelativePath = fs::relative(CF.Filename, Meta.ProjectPath);
    if (util::starts_with(RelativePath, "./")) {
      RelativePath.erase(0, 2);
    }
    ConflictPaths.push_back(std::move(RelativePath));
  }
  auto Start = tbb::tick_count::now();
  GraphBuilder OurBuilder(Side::OURS, Meta, ConflictPaths, ST.OurSourceList,
                          ST.OurDirectIncluded);
  GraphBuilder BaseBuilder(Side::BASE, Meta, ConflictPaths, ST.BaseSourceList,
                           ST.BaseDirectIncluded);
  GraphBuilder TheirBuilder(Side::THEIRS, Meta, ConflictPaths,
                            ST.TheirSourceList, ST.TheirDirectIncluded);

  tbb::task_group TG;
  bool OurOk = false;
  bool BaseOk = false;
  bool TheirOk = false;
  TG.run([&OurBuilder, &OurOk]() { OurOk = OurBuilder.build(); });
  TG.run([&BaseBuilder, &BaseOk]() { BaseOk = BaseBuilder.build(); });
  TG.run([&TheirBuilder, &TheirOk]() { TheirOk = TheirBuilder.build(); });
  TG.wait();
  auto End = tbb::tick_count::now();
  if (!OurOk || !TheirOk) {
    spdlog::info("fail to construct graph representation of revisions");
    return;
  }
  spdlog::info("it takes {} ms to build 3 commit nodes' graph representation ",
               (End - Start).seconds() * 1000);
}

/**
 * @brief use clang libTooling library to infer missing compile commands from
 * JSON CDB
 */
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
