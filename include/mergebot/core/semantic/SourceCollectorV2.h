//
// Created by whalien on 05/05/23.
//

#ifndef MB_SOURCECOLLECTORV2_H
#define MB_SOURCECOLLECTORV2_H

#include "mergebot/core/handler/SAHandler.h"
#include "mergebot/core/model/Side.h"
#include "mergebot/core/model/SimplifiedDiffDelta.h"
#include <clang/Tooling/CompilationDatabase.h>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
namespace mergebot {
namespace sa {
class SourceCollectorV2 {
public:
  struct AnalysisSourceTuple {
    std::vector<std::string> OurSourceList;
    std::vector<std::string> BaseSourceList;
    std::vector<std::string> TheirSourceList;
  };

  /// default settings
  explicit SourceCollectorV2(ProjectMeta const &Meta) : Meta(Meta) {}
  /// only want to analysis both modified file sets
  SourceCollectorV2(ProjectMeta const &Meta, bool OnlyBothModified)
      : Meta(Meta), OnlyBothModified(OnlyBothModified) {}
  /// when we want to collect all the included files for context analysis
  SourceCollectorV2(ProjectMeta const &Meta,
                    std::shared_ptr<clang::tooling::CompilationDatabase> const
                        &OurCompilations,
                    std::shared_ptr<clang::tooling::CompilationDatabase> const
                        &BaseCompilations,
                    std::shared_ptr<clang::tooling::CompilationDatabase> const
                        &TheirCompilations)
      : Meta(Meta), LookupIncluded(true), OurCompilations(OurCompilations),
        BaseCompilations(BaseCompilations),
        TheirCompilations(TheirCompilations) {}

  void collectAnalysisSources();

  /// boilerplate accessors and mutators
  bool lookupIncluded() const { return LookupIncluded; }
  bool onlyBothModified() const { return OnlyBothModified; }
  AnalysisSourceTuple analysisSourceTuple() const { return SourceTuple; }
  void setLookupIncluded(bool LI) { LookupIncluded = LI; }
  void setOnlyBothModified(bool OBM) { OnlyBothModified = OBM; }
  void setAnalysisSourceTuple(AnalysisSourceTuple const &ST) {
    SourceTuple = ST;
  }

private:
  AnalysisSourceTuple diffDeltaIntersection(
      std::unordered_set<SimplifiedDiffDelta> &OurDiffDeltas,
      std::unordered_set<SimplifiedDiffDelta> &TheirDiffDeltas) const;
  void diffDeltaWithHeuristic(
      std::unordered_set<std::string> &NewSourceSet,
      std::unordered_set<std::string> &OldSourceSet,
      std::unordered_set<SimplifiedDiffDelta> const &DiffDeltas) const;

  void extendIncludedSources(
      Side S, std::vector<std::string> &SourceList,
      std::shared_ptr<clang::tooling::CompilationDatabase> Compilations) const;

  ProjectMeta Meta;
  bool LookupIncluded = false;
  bool OnlyBothModified = false;
  AnalysisSourceTuple SourceTuple;
  std::shared_ptr<clang::tooling::CompilationDatabase> OurCompilations;
  std::shared_ptr<clang::tooling::CompilationDatabase> BaseCompilations;
  std::shared_ptr<clang::tooling::CompilationDatabase> TheirCompilations;
};
} // namespace sa
} // namespace mergebot

#endif // MB_SOURCECOLLECTORV2_H
