//
// Created by whalien on 05/05/23.
//

#include "mergebot/core/semantic/SourceCollectorV2.h"
#include "mergebot/core/model/SimplifiedDiffDelta.h"
#include "mergebot/filesystem.h"
#include "mergebot/magic_enum_customization.h"
#include "mergebot/utils/gitservice.h"
#include "mergebot/utils/stringop.h"
#include <clang/Basic/Diagnostic.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <spdlog/spdlog.h>
#include <unordered_set>
#include <vector>

namespace mergebot {
namespace sa {
void SourceCollectorV2::collectAnalysisSources() {
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
                          OurCompilations);
    extendIncludedSources(Side::BASE, SourceTuple.BaseSourceList,
                          BaseCompilations);
    extendIncludedSources(Side::THEIRS, SourceTuple.TheirSourceList,
                          TheirCompilations);
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
      spdlog::warn("source file{} of merge scenario{} in project{} should "
                   "not appear in git diff, which is weird",
                   SDD.toString(), Meta.MS.name, Meta.Project);
      break;
    }
  }
}
void SourceCollectorV2::extendIncludedSources(
    Side S, std::vector<std::string> &SourceList,
    std::shared_ptr<clang::tooling::CompilationDatabase> Compilations) const {
  spdlog::info("side: {}", magic_enum::enum_name(S));
  fs::path SourceDir =
      fs::path(Meta.ProjectCacheDir) / Meta.MS.name / magic_enum::enum_name(S);
  std::vector<std::string> SourcePaths;
  SourcePaths.reserve(SourceList.size());
  for (const auto &RelativeSource : SourceList) {
    SourcePaths.push_back(SourceDir / RelativeSource);
  }
  clang::tooling::ClangTool Tool(*Compilations, SourcePaths);
  Tool.setPrintErrorMessage(false);
  Tool.setDiagnosticConsumer(new clang::IgnoringDiagConsumer());
  std::vector<std::unique_ptr<clang::ASTUnit>> TUs;
  Tool.buildASTs(TUs);
  for (const std::unique_ptr<clang::ASTUnit> &TU : TUs) {
    clang::Preprocessor &PP = TU->getPreprocessor();
    clang::Preprocessor::IncludedFilesSet &IncludedFiles =
        PP.getIncludedFiles();
    for (const auto *FileEntry : IncludedFiles) {
      if (fs::path(FileEntry->getName().str()).is_relative()) {
        std::error_code EC;
        if (!EC) {
          std::string Filename = FileEntry->getName().str();
          if (util::starts_with(Filename, "./")) {
            Filename = Filename.substr(2);
          }
          SourceList.push_back(Filename);
        }
      }
    }
  }
}
} // namespace sa
} // namespace mergebot
