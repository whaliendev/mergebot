//
// Created by whalien on 05/05/23.
//

#include "mergebot/core/semantic/SourceCollectorV2.h"
#include "mergebot/core/magic_enum_customization.h"
#include "mergebot/core/model/SimplifiedDiffDelta.h"
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
#include <spdlog/spdlog.h>
#include <unordered_set>
#include <vector>

namespace mergebot {
namespace sa {
void SourceCollectorV2::collectAnalysisSources() {
  // retrieve concise diff deltas from the git repo
  std::unordered_set<SimplifiedDiffDelta> OurDiffDeltas =
      util::list_cpp_diff_files(Meta.ProjectPath, Meta.MS.base, Meta.MS.ours);
  std::unordered_set<SimplifiedDiffDelta> TheirDiffDeltas =
      util::list_cpp_diff_files(Meta.ProjectPath, Meta.MS.base, Meta.MS.ours);

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

  if (LookupIncluded) {
    assert(OurCompilations && BaseCompilations && TheirCompilations &&
           "CompDB should not be null if we want to to deps analysis");
    extendIncludedSources(Side::OURS, SourceTuple.OurSourceList,
                          SourceTuple.OurDirectIncluded, OurCompilations);
    extendIncludedSources(Side::BASE, SourceTuple.BaseSourceList,
                          SourceTuple.BaseDirectIncluded, BaseCompilations);
    extendIncludedSources(Side::THEIRS, SourceTuple.TheirSourceList,
                          SourceTuple.TheirDirectIncluded, TheirCompilations);
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
  for (SimplifiedDiffDelta const &SDD : DiffDeltas) {
    switch (SDD.Type) {
    case SimplifiedDiffDelta::DeltaType::ADDED:
      NewSourceSet.insert(SDD.NewPath);
      break;
    case SimplifiedDiffDelta::DELETED:
      OldSourceSet.insert(SDD.OldPath);
      break;
    case SimplifiedDiffDelta::MODIFIED:
    case SimplifiedDiffDelta::RENAMED:
    case SimplifiedDiffDelta::COPIED:
      NewSourceSet.insert(SDD.NewPath);
      OldSourceSet.insert(SDD.OldPath);
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
}

void SourceCollectorV2::extendIncludedSources(
    Side S, std::vector<std::string> const &SourceList,
    std::unordered_map<std::string, std::vector<std::string>> &IncludeMap,
    std::shared_ptr<clang::tooling::CompilationDatabase> Compilations) const {
  spdlog::info(
      "we're collecting {} side's direct included headers, which may take "
      "some time...",
      magic_enum::enum_name(S));
  fs::path SourceDir =
      fs::path(Meta.ProjectCacheDir) / Meta.MS.name / magic_enum::enum_name(S);
  std::vector<std::string> SourcePaths;
  SourcePaths.reserve(SourceList.size());
  for (const auto &RelativeSource : SourceList) {
    SourcePaths.push_back(SourceDir / RelativeSource);
  }

  for (size_t i = 0; i < SourcePaths.size(); ++i) {
    std::string const &SourcePath = SourcePaths[i];
    //    spdlog::debug("collecting direct included headers for file: {}",
    //                  SourceList[i]);
    clang::tooling::ClangTool Tool(*Compilations, SourcePath);
    Tool.setPrintErrorMessage(false);
    Tool.setDiagnosticConsumer(new clang::IgnoringDiagConsumer());

    std::unique_ptr<IncludeLookupActionFactory> ActionFactory =
        std::make_unique<IncludeLookupActionFactory>();
    Tool.run(ActionFactory.get());
    //    spdlog::debug("{}'s direct included files: {}", SourceList[i],
    //                 fmt::join(ActionFactory->includedFiles(), "\t"));
    IncludeMap[SourceList[i]] = ActionFactory->includedFiles();
  }
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
  // currently, we don't analyze system library. Besides, here we leverage a
  // implementation detail  of clang preprocessor to perform direct included
  // headers analysis, namely: clang preprocessor will do a DFS when
  // preprocessing includes
  if (IsAngled || Depth != 1) {
    return;
  }
  if (File) {
    const auto &FileEntry = File->getFileEntry();
    if (util::starts_with(FileEntry.getName(), "./")) {
      DirectIncludes.insert(FileEntry.getName().substr(2).str());
    }
  }
}

std::vector<std::string> IncludeLookupActionFactory::IncludedFiles;
} // namespace sa
} // namespace mergebot
