//
// Created by whalien on 04/05/23.
//

#ifndef MB_SOURCEFILECOLLECTOR_H
#define MB_SOURCEFILECOLLECTOR_H
#include "mergebot/core/handler/SAHandler.h"
#include "mergebot/core/model/Side.h"
#include "mergebot/core/model/SimplifiedDiffDelta.h"
#include <clang/Tooling/CompilationDatabase.h>
#include <string>
#include <vector>

namespace mergebot {
namespace sa {
struct MergeScenarioFileSet {
  explicit MergeScenarioFileSet(ProjectMeta const &Meta) : Meta(Meta) {}
  /// project and merge scenario meta info, optional
  ProjectMeta Meta;
  std::vector<SimplifiedDiffDelta> OurDiffDeltas;
  std::vector<SimplifiedDiffDelta> BaseDiffDeltas;
  std::vector<SimplifiedDiffDelta> TheirDiffDeltas;
  std::vector<SimplifiedDiffDelta> BothModifiedDiffDeltas;
};

class SourceFileCollector {
public:
  /// do not want to analysis included files
  explicit SourceFileCollector(ProjectMeta const &Meta)
      : Meta(Meta), FileSet(Meta) {}

  /// when we want to collect included files
  SourceFileCollector(ProjectMeta const &Meta,
                      std::shared_ptr<clang::tooling::CompilationDatabase> const
                          &OurCompilations,
                      std::shared_ptr<clang::tooling::CompilationDatabase> const
                          &BaseCompilations,
                      std::shared_ptr<clang::tooling::CompilationDatabase> const
                          &TheirCompilations)
      : Meta(Meta), FileSet(Meta), LookupIncluded(true),
        OurCompilations(OurCompilations), BaseCompilations(BaseCompilations),
        TheirCompilations(TheirCompilations) {}

  /// the return value is stored in FileSet
  void collectAnalysisSources();
  //  void
  //  collectSourcesOfOneSide(Side S,
  //                          std::vector<SimplifiedDiffDelta> const
  //                          &DiffDeltas);

  // boilerplate getters and setters
  void setOnlyBothModified(bool OBM) { OnlyBothModified = OBM; }
  void setLookupIncluded(bool CI) { LookupIncluded = CI; }
  MergeScenarioFileSet mergeScenarioFileSet() const { return FileSet; }

private:
  /// collect included files based on our collected modified sources
  void collectIncludedFiles(
      Side S, std::vector<SimplifiedDiffDelta> const &DiffDeltas,
      std::shared_ptr<clang::tooling::CompilationDatabase> const &Compilations);
  void computeDiffDeltas();
  /// meta info of project and merge scenario
  ProjectMeta Meta;
  /// merge scenario file sets to collect source to analysis
  MergeScenarioFileSet FileSet;
  /// whether we need to copy included sources to analysis
  bool LookupIncluded = false;
  /// compilations to look up included files
  std::shared_ptr<clang::tooling::CompilationDatabase> OurCompilations;
  std::shared_ptr<clang::tooling::CompilationDatabase> BaseCompilations;
  std::shared_ptr<clang::tooling::CompilationDatabase> TheirCompilations;
  /// only care about source files modified by both side, for textual analysis
  bool OnlyBothModified = false;
};
} // namespace sa
} // namespace mergebot

#endif // MB_SOURCEFILECOLLECTOR_H
