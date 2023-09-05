//
// Created by whalien on 05/05/23.
//

#include "mergebot/core/semantic/SourceCollectorV2.h"
#include "mergebot/core/magic_enum_customization.h"
#include "mergebot/core/model/SimplifiedDiffDelta.h"
#include "mergebot/core/sa_utility.h"
#include "mergebot/filesystem.h"
#include "mergebot/utils/gitservice.h"
#include "mergebot/utils/stringop.h"
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <memory>
#include <mutex>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/parallel_invoke.h>
#include <spdlog/spdlog.h>
#include <unordered_set>
#include <vector>

namespace mergebot {
namespace sa {
void SourceCollectorV2::collectAnalysisSources() {
  // retrieve concise diff deltas from the git repo
  // note that git_diff_find_similar is very slow when diff deltas are large, do
  // we really need rename/copy detection?
  auto [OurElapsed, OurDiffDeltas] = utils::MeasureRunningTime(
      util::list_cpp_diff_files, Meta.ProjectPath, Meta.MS.base, Meta.MS.ours);
  spdlog::info("it takes {} ms to list cpp files in ours", OurElapsed);
  auto [TheirElapsed, TheirDiffDeltas] =
      utils::MeasureRunningTime(util::list_cpp_diff_files, Meta.ProjectPath,
                                Meta.MS.base, Meta.MS.theirs);
  spdlog::info("it takes {} ms to list cpp files in theirs", TheirElapsed);

  if (OnlyBothModified) {
    SourceTuple = diffDeltaIntersection(OurDiffDeltas, TheirDiffDeltas);
  } else {
    std::unordered_set<std::string> OurSourceSet;
    std::unordered_set<std::string> BaseSourceSet;
    std::unordered_set<std::string> TheirSourceSet;
    // use diff deltas to fill source set
    diffDeltaWithHeuristic(OurSourceSet, BaseSourceSet, OurDiffDeltas);
    diffDeltaWithHeuristic(TheirSourceSet, BaseSourceSet, TheirDiffDeltas);
    SourceTuple = {
        .OurSourceList = std::vector(OurSourceSet.begin(), OurSourceSet.end()),
        .BaseSourceList =
            std::vector(BaseSourceSet.begin(), BaseSourceSet.end()),
        .TheirSourceList =
            std::vector(TheirSourceSet.begin(), TheirSourceSet.end())};
  }

  // heavy op for long-lived unmerged branches
  if (LookupIncluded) {
    assert(OurCompilations && BaseCompilations && TheirCompilations &&
           "CompDB should not be null if we want to to deps analysis");
    SourceTuple.OurDirectIncluded.reserve(SourceTuple.OurSourceList.size());
    SourceTuple.BaseDirectIncluded.reserve(SourceTuple.BaseSourceList.size());
    SourceTuple.TheirDirectIncluded.reserve(SourceTuple.TheirSourceList.size());

    tbb::parallel_invoke([this]() { extendIncludedSources(Side::OURS); },
                         [this]() { extendIncludedSources(Side::BASE); },
                         [this]() { extendIncludedSources(Side::THEIRS); });
  }
}

SourceCollectorV2::AnalysisSourceTuple SourceCollectorV2::diffDeltaIntersection(
    std::unordered_set<SimplifiedDiffDelta> &OurDiffDeltas,
    std::unordered_set<SimplifiedDiffDelta> &TheirDiffDeltas) const {
  std::unordered_set<std::string> Intersection;
  std::unordered_set<SimplifiedDiffDelta> &Smaller = OurDiffDeltas;
  std::unordered_set<SimplifiedDiffDelta> &Bigger = TheirDiffDeltas;
  if (Smaller.size() > Bigger.size()) {
    swap(Smaller, Bigger);
  }
  for (SimplifiedDiffDelta const &SDD : Smaller) {
    if (SDD.Type == SimplifiedDiffDelta::DeltaType::MODIFIED &&
        Bigger.count(SDD) > 0) {
      Intersection.insert(SDD.NewPath);
    }
  }
  return {
      .OurSourceList = std::vector(Intersection.begin(), Intersection.end()),
      .BaseSourceList = std::vector(Intersection.begin(), Intersection.end()),
      .TheirSourceList = std::vector(Intersection.begin(), Intersection.end())};
}

void SourceCollectorV2::diffDeltaWithHeuristic(
    std::unordered_set<std::string> &NewSourceSet,
    std::unordered_set<std::string> &OldSourceSet,
    std::unordered_set<SimplifiedDiffDelta> const &DiffDeltas) const {
  int AddedCnt = 0, DeletedCnt = 0, ModifiedCnt = 0;
  for (SimplifiedDiffDelta const &SDD : DiffDeltas) {
    switch (SDD.Type) {
    case SimplifiedDiffDelta::DeltaType::ADDED:
      NewSourceSet.insert(SDD.NewPath);
      AddedCnt++;
      break;
    case SimplifiedDiffDelta::DELETED:
      OldSourceSet.insert(SDD.OldPath);
      DeletedCnt++;
      break;
    case SimplifiedDiffDelta::MODIFIED:
    case SimplifiedDiffDelta::RENAMED:
    case SimplifiedDiffDelta::COPIED:
      NewSourceSet.insert(SDD.NewPath);
      OldSourceSet.insert(SDD.OldPath);
      ModifiedCnt++;
      break;
    case SimplifiedDiffDelta::UNMODIFIED:
    case SimplifiedDiffDelta::IGNORED:
    case SimplifiedDiffDelta::UNTRACKED:
    case SimplifiedDiffDelta::TYPECHANGE:
    case SimplifiedDiffDelta::UNREADABLE:
    case SimplifiedDiffDelta::CONFLICTED:
      spdlog::warn("source file{}(modify kind: {}) of merge scenario{} in "
                   "project{} should "
                   "not appear in git diff, which is weird",
                   SDD.toString(), magic_enum::enum_name(SDD.Type),
                   Meta.MS.name, Meta.Project);
      break;
    }
  }
  spdlog::debug("added: {}, deleted: {}, modified: {}", AddedCnt, DeletedCnt,
                ModifiedCnt);
}

void SourceCollectorV2::extendIncludedSources(Side S) {

  spdlog::info("collecting {} side's direct included headers, which may take "
               "some time...",
               magic_enum::enum_name(S));
  const auto &SourceList = S == Side::OURS   ? SourceTuple.OurSourceList
                           : S == Side::BASE ? SourceTuple.BaseSourceList
                                             : SourceTuple.TheirSourceList;
  auto &IncludeMap = S == Side::OURS   ? SourceTuple.OurDirectIncluded
                     : S == Side::BASE ? SourceTuple.BaseDirectIncluded
                                       : SourceTuple.TheirDirectIncluded;
  std::shared_ptr<clang::tooling::CompilationDatabase> Compilations =
      S == Side::OURS   ? OurCompilations
      : S == Side::BASE ? BaseCompilations
                        : TheirCompilations;

  fs::path SourceDir =
      fs::path(Meta.ProjectCacheDir) / Meta.MS.name / magic_enum::enum_name(S);

  std::vector<std::string> SourcePaths;
  SourcePaths.reserve(SourceList.size());

  for (const auto &RelativeSource : SourceList) {
    SourcePaths.push_back(SourceDir / RelativeSource);
  }

  //  for (size_t i = 0; i < SourcePaths.size(); ++i) {
  //    std::string const &SourcePath = SourcePaths[i];
  //    clang::tooling::ClangTool Tool(*Compilations, SourcePath);
  //    Tool.setPrintErrorMessage(false);
  //    Tool.setDiagnosticConsumer(new clang::IgnoringDiagConsumer());
  //
  //    std::unique_ptr<IncludeLookupActionFactory> ActionFactory =
  //        std::make_unique<IncludeLookupActionFactory>();
  //    Tool.run(ActionFactory.get());
  //
  //    IncludeMap.insert({SourceList[i], ActionFactory->copyIncludedFiles()});
  //  }

  std::mutex includeMapMutex; // Mutex for synchronizing access to IncludeMap

  tbb::parallel_for(static_cast<size_t>(0), SourcePaths.size(), [&](size_t i) {
    std::string const &SourcePath = SourcePaths[i];

    clang::tooling::ClangTool Tool(*Compilations, SourcePath);
    Tool.setPrintErrorMessage(false);
    Tool.setDiagnosticConsumer(new clang::IgnoringDiagConsumer());

    std::unique_ptr<IncludeLookupActionFactory> ActionFactory =
        std::make_unique<IncludeLookupActionFactory>();
    Tool.run(ActionFactory.get());

    std::lock_guard<std::mutex> lock(includeMapMutex);
    IncludeMap[SourceList[i]] = ActionFactory->copyIncludedFiles();
  });
}

void IncludeLookupCallback::LexedFileChanged(
    clang::FileID FID, clang::PPCallbacks::LexedFileChangeReason Reason,
    clang::SrcMgr::CharacteristicKind FileType, clang::FileID PrevFID,
    clang::SourceLocation Loc) {
  switch (Reason) {
  case clang::PPCallbacks::LexedFileChangeReason::EnterFile:
    Depth++;
    break;
  case LexedFileChangeReason::ExitFile:
    Depth--;
    break;
  }
}
void IncludeLookupCallback::InclusionDirective(
    clang::SourceLocation HashLoc, const clang::Token &IncludeTok,
    llvm::StringRef FileName, bool IsAngled,
    clang::CharSourceRange FilenameRange, clang::OptionalFileEntryRef File,
    llvm::StringRef SearchPath, llvm::StringRef RelativePath,
    const clang::Module *Imported, clang::SrcMgr::CharacteristicKind FileType) {
  // currently, we don't analyze system library. Besides, here we leverage
  // a implementation detail  of clang preprocessor to perform direct
  // included headers analysis, namely: clang preprocessor will do a DFS
  // when preprocessing includes
  if (IsAngled || Depth != 1) {
    return;
  }
  if (File) {
    const auto &FileEntry = File->getFileEntry();
    assert(!util::starts_with(FileEntry.getName(), "./") &&
           "Direct Include Path should not be relative");
    DirectIncludes.insert(FileEntry.getName().str());
  }
}

std::vector<std::string> IncludeLookupActionFactory::IncludedFiles;
} // namespace sa
} // namespace mergebot
