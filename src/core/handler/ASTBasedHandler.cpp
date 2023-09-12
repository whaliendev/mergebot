//
// Created by whalien on 13/03/23.
//

#include "mergebot/core/handler/ASTBasedHandler.h"
#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/core/semantic/GraphBuilder.h"
#include "mergebot/core/semantic/SourceCollectorV2.h"
#include "mergebot/filesystem.h"
#include "mergebot/utils/stringop.h"
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/JSONCompilationDatabase.h>
#include <filesystem>
#include <llvm/ADT/StringRef.h>
#include <oneapi/tbb/parallel_invoke.h>
#include <oneapi/tbb/tick_count.h>
#include <spdlog/spdlog.h>
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

  spdlog::info("Resolving conflicts using AST based handler...");
  //  spdlog::info("dependencies analysis disabled due to lack of CompDB");

  /// init CompDB
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
  auto Start = tbb::tick_count::now();
  SC.collectAnalysisSources();
  auto End = tbb::tick_count::now();
  spdlog::info("it takes {} ms to collect collection of sources to analyze",
               (End - Start).seconds() * 1000);
  SourceCollectorV2::AnalysisSourceTuple ST = SC.analysisSourceTuple();
  spdlog::debug("source list size: {}, {}, {}, first direct include size: {}, "
                "{}, {}(our, base, their).",
                ST.OurSourceList.size(), ST.BaseSourceList.size(),
                ST.TheirSourceList.size(),
                ST.OurDirectIncluded.begin()->second.size(),
                ST.BaseDirectIncluded.begin()->second.size(),
                ST.TheirDirectIncluded.begin()->second.size());

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
  Start = tbb::tick_count::now();
  GraphBuilder OurBuilder(Side::OURS, Meta, ConflictPaths, ST.OurSourceList,
                          ST.OurDirectIncluded);
  GraphBuilder BaseBuilder(Side::BASE, Meta, ConflictPaths, ST.BaseSourceList,
                           ST.BaseDirectIncluded);
  GraphBuilder TheirBuilder(Side::THEIRS, Meta, ConflictPaths,
                            ST.TheirSourceList, ST.TheirDirectIncluded);

  bool OurOk = false;
  bool BaseOk = false;
  bool TheirOk = false;
  OurOk = OurBuilder.build();
  BaseOk = BaseBuilder.build();
  TheirOk = TheirBuilder.build();
  //  tbb::parallel_invoke([&]() { OurOk = OurBuilder.build(); },
  //                       [&]() { BaseOk = BaseBuilder.build(); },
  //                       [&]() { TheirOk = TheirBuilder.build(); });
  End = tbb::tick_count::now();
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
  tbb::parallel_invoke(
      [&]() {
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
      },
      [&]() {
        std::string ErrMsg;
        BaseCompilationsPair.first =
            clang::tooling::inferMissingCompileCommands(
                clang::tooling::JSONCompilationDatabase::loadFromFile(
                    BaseCompDB, ErrMsg,
                    clang::tooling::JSONCommandLineSyntax::AutoDetect));
        BaseCompilationsPair.second = true;
        if (!ErrMsg.empty()) {
          spdlog::error(ErrMsg, BaseCompDB);
          BaseCompilationsPair.second = false;
        }
      },
      [&]() {
        std::string ErrMsg;
        TheirCompilationsPair.first =
            clang::tooling::inferMissingCompileCommands(
                clang::tooling::JSONCompilationDatabase::loadFromFile(
                    TheirCompDB, ErrMsg,
                    clang::tooling::JSONCommandLineSyntax::AutoDetect));
        TheirCompilationsPair.second = true;
        if (!ErrMsg.empty()) {
          spdlog::error(ErrMsg, TheirCompDB);
          TheirCompilationsPair.second = false;
        }
      });
}

} // namespace sa
} // namespace mergebot
