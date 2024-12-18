//
// Created by whalien on 13/03/23.
//
#define MB_EXPORT_GRAPH

#ifdef MB_EXPORT_GRAPH
#include "mergebot/core/semantic/graph_export.h"
#endif
#include "mergebot/core/handler/ASTBasedHandler.h"
#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/core/semantic/GraphBuilder.h"
#include "mergebot/core/semantic/GraphMerger.h"
#include "mergebot/core/semantic/SourceCollectorV2.h"
#include "mergebot/core/semantic/pretty_printer.h"
#include "mergebot/filesystem.h"
#include "mergebot/utils/fileio.h"
#include "mergebot/utils/gitservice.h"
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

  // 0. gets relative paths of conflict files, do preparations
  std::vector<std::string> ConflictPaths;
  ConflictPaths.reserve(ConflictFiles.size());
  std::transform(ConflictFiles.begin(), ConflictFiles.end(),
                 std::back_inserter(ConflictPaths),
                 [&](ConflictFile const &CF) {
                   const std::string RelativePath =
                       fs::relative(CF.Filename, Meta.ProjectPath).string();
                   if (util::starts_with(RelativePath, "./") ||
                       util::starts_with(RelativePath, ".\\")) {
                     return RelativePath.substr(2);
                   } else {
                     return RelativePath;
                   }
                 });
  auto [OurSideId, BaseSideId, TheirSideId] = extractSideIdentifiers(
      (fs::path(Meta.MSCacheDir) / "conflicts").string());
  if (OurSideId.empty() || TheirSideId.empty()) {
    OurSideId = "HEAD";
    BaseSideId = Meta.MS.base;
    TheirSideId = Meta.MS.theirs;
  }

  /// do variant intelli merge
  // 1. Collect Sources set to analyze
  // note that LookupIncluded is default enabled, OnlyBothModified is
  // default disabled
  SourceCollectorV2 SC(Meta, OurCompilations, BaseCompilations,
                       TheirCompilations);
  auto Start = tbb::tick_count::now();
  SC.collectAnalysisSourcesV2(ConflictPaths);
  auto End = tbb::tick_count::now();
  spdlog::info("it takes {} ms to collect collection of sources to analyze",
               (End - Start).seconds() * 1000);
  SourceCollectorV2::AnalysisSourceTuple ST = SC.analysisSourceTuple();
  //  spdlog::debug("source list size: {}, {}, {}, first direct include size:
  //  {}, "
  //                "{}, {}(our, base, their).",
  //                ST.OurSourceList.size(), ST.BaseSourceList.size(),
  //                ST.TheirSourceList.size(),
  //                ST.OurDirectIncluded.begin()->second.size(),
  //                ST.BaseDirectIncluded.begin()->second.size(),
  //                ST.TheirDirectIncluded.begin()->second.size());

  // 2. Get Graph representation of 3 commit nodes
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
  //  OurOk = OurBuilder.build();
  //  BaseOk = BaseBuilder.build();
  //  TheirOk = TheirBuilder.build();
  tbb::parallel_invoke([&]() { OurOk = OurBuilder.build(); },
                       [&]() { BaseOk = BaseBuilder.build(); },
                       [&]() { TheirOk = TheirBuilder.build(); });
  End = tbb::tick_count::now();
  if (!OurOk || !TheirOk) {
    spdlog::info("fail to construct graph representation of revisions");
    return;
  }
  spdlog::info("graph built successfully, stat:\nOur:\n\t{} nodes, {} edges\n"
               "Base:\n\t{} nodes, {} edges\nTheir:\n\t{} nodes, {} edges",
               OurBuilder.numVertices(), OurBuilder.numEdges(),
               BaseBuilder.numVertices(), BaseBuilder.numEdges(),
               TheirBuilder.numVertices(), TheirBuilder.numEdges());
  spdlog::info("it takes {} ms to build 3 commit nodes' graph representation ",
               (End - Start).seconds() * 1000);

#ifdef MB_EXPORT_GRAPH
  const fs::path IntermediateGraphsDir = fs::path(Meta.MSCacheDir) / "graphs";
  fs::create_directories(IntermediateGraphsDir);
  const std::string OurDotDest =
      fmt::format("{}.dot", (fs::path(IntermediateGraphsDir) /
                             magic_enum::enum_name(Side::OURS))
                                .string());
  const std::string BaseDotDest =
      fmt::format("{}.dot", (fs::path(IntermediateGraphsDir) /
                             magic_enum::enum_name(Side::BASE))
                                .string());
  const std::string TheirDotDest =
      fmt::format("{}.dot", (fs::path(IntermediateGraphsDir) /
                             magic_enum::enum_name(Side::THEIRS))
                                .string());
  ExportGraphToDot(OurBuilder.graph(), OurDotDest, true);
  ExportGraphToDot(OurBuilder.graph(), BaseDotDest, true);
  ExportGraphToDot(OurBuilder.graph(), TheirDotDest, true);
  spdlog::info("export intermediate graphs to {}",
               IntermediateGraphsDir.string());
#endif

  // 3. do top-down, bottom-up match
  Start = tbb::tick_count::now();
  GraphMerger Merger(Meta, OurBuilder.graph(), BaseBuilder.graph(),
                     TheirBuilder.graph(), OurSideId, BaseSideId, TheirSideId);
  Merger.threeWayMatch();
  End = tbb::tick_count::now();
  spdlog::info("it takes {} ms to match three revision graphs",
               (End - Start).seconds() * 1000);

  // 4. recursively merges matched TranslationUnits
  Start = tbb::tick_count::now();
  std::vector<std::string> MergedFiles = Merger.threeWayMerge();
  End = tbb::tick_count::now();
  spdlog::info("it takes {} ms to merge three revision graphs, merged sources "
               "destination: {}",
               (End - Start).seconds() * 1000, Merger.getMergedDir());

  //  // 5. get diff hunks and serialize it
  //  const std::string PatchesDir =
  //      fs::path(Meta.MSCacheDir) / "resolutions" / "patches";
  //  fs::create_directories(PatchesDir);
  //  std::for_each(ConflictFiles.begin(), ConflictFiles.end(), [&](auto &CF) {
  //    std::string MSCacheDir = Meta.MSCacheDir;
  //    auto Relative = fs::relative(CF.Filename, Meta.ProjectPath);
  //    const std::string DestPatchFile = fs::path(PatchesDir) / Relative;
  //    fs::create_directories(fs::path(DestPatchFile).parent_path());
  //
  //    std::string MergedFile =
  //        (fs::path(MSCacheDir) / "merged" / Relative).string();
  //    if (!fs::exists(CF.Filename) || !fs::exists(MergedFile)) {
  //      spdlog::warn("cannot find conflict file or merged file, skip diff
  //      hunks "
  //                   "generation for file [{}]",
  //                   CF.Filename);
  //      return;
  //    }
  //
  //    auto DiffHunks = util::get_git_diff_hunks(CF.Filename, MergedFile);
  //
  //    nlohmann::json DiffHunksJson = DiffHunks;
  //    bool Success =
  //        util::file_overwrite_content_sync(DestPatchFile,
  //        DiffHunksJson.dump(2));

  //    std::for_each(DiffHunks.begin(), DiffHunks.end(), [&](auto &Hunk) {
  //      spdlog::debug("hunk: start: {}, offset: {}, content: {}",
  //      Hunk.start,
  //                    Hunk.offset, Hunk.content);
  //    });

  //    if (!Success) {
  //      spdlog::warn("fail to write diff hunks to file [{}]", DestPatchFile);
  //    }
  //  });
}

namespace details {
void scanLines(const std::string &Filename, std::string &OurSideId,
               std::string &BaseSideId, std::string &TheirSideId) {
  std::ifstream File(Filename);
  if (!File.is_open()) {
    spdlog::warn("fail to open file [{}] for extracting side identifiers",
                 Filename);
    return;
  }
  std::string line;
  while (std::getline(File, line)) {
    if (util::starts_with(line, "<<<<<<<")) {
      auto parts = util::string_split(line, " ");
      if (parts.size() >= 2) {
        OurSideId = parts[1];
      }
    }

    if (util::starts_with(line, "|||||||")) {
      auto parts = util::string_split(line, " ");
      if (parts.size() >= 2) {
        BaseSideId = parts[1];
      }
    }

    if (util::starts_with(line, ">>>>>>>")) {
      auto parts = util::string_split(line, " ");
      if (parts.size() >= 2) {
        TheirSideId = parts[1];
        break;
      }
    }
  }
}
} // namespace details

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

std::tuple<std::string, std::string, std::string>
ASTBasedHandler::extractSideIdentifiers(const std::string &ConflictDir) const {
  std::string OurSideId;
  std::string BaseSideId;
  std::string TheirSideId;
  std::error_code EC;
  size_t CheckedFiles = 0;
  for (const auto &Entry : fs::directory_iterator(ConflictDir, EC)) {
    if (EC) {
      spdlog::warn("fail to iterate directory [{}], error message: {}",
                   ConflictDir, EC.message());
      continue;
    }
    if (Entry.is_regular_file()) {
      if (CheckedFiles >= 5)
        break;
      const std::string Filename = Entry.path().string();
      details::scanLines(Filename, OurSideId, BaseSideId, TheirSideId);
      if (!OurSideId.empty() && !TheirSideId.empty()) {
        break;
      }
      CheckedFiles++;
    }
  }
  return {OurSideId, BaseSideId, TheirSideId};
}

} // namespace sa
} // namespace mergebot
