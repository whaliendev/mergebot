//
// Created by whalien on 04/05/23.
//

#include "mergebot/core/semantic/SourceFileCollector.h"
#include "mergebot/core/model/SimplifiedDiffDelta.h"
#include "mergebot/utils/gitservice.h"
#include <algorithm>
#include <clang/Tooling/CompilationDatabase.h>
#include <unordered_set>
#include <vector>

namespace mergebot {
namespace sa {
void SourceFileCollector::collectAnalysisSources() {
  computeDiffDeltas();
  if (LookupIncluded) {
    if (OnlyBothModified) {
      collectIncludedFiles(Side::OURS, FileSet.BothModifiedDiffDeltas,
                           OurCompilations);
      collectIncludedFiles(Side::BASE, FileSet.BothModifiedDiffDeltas,
                           BaseCompilations);
      collectIncludedFiles(Side::THEIRS, FileSet.BothModifiedDiffDeltas,
                           TheirCompilations);
    } else {
      collectIncludedFiles(Side::OURS, FileSet.OurDiffDeltas, OurCompilations);
      collectIncludedFiles(Side::BASE, FileSet.BaseDiffDeltas,
                           BaseCompilations);
      collectIncludedFiles(Side::THEIRS, FileSet.TheirDiffDeltas,
                           TheirCompilations);
    }
  }
}

void SourceFileCollector::computeDiffDeltas() {
  std::unordered_set<SimplifiedDiffDelta> OurDiffDeltas =
      util::list_cpp_diff_files(Meta.ProjectPath, Meta.MS.base, Meta.MS.ours);
  std::unordered_set<SimplifiedDiffDelta> TheirDiffDeltas =
      util::list_cpp_diff_files(Meta.ProjectPath, Meta.MS.base, Meta.MS.ours);

  std::unordered_set<SimplifiedDiffDelta> OurModified;
  std::copy_if(OurDiffDeltas.begin(), OurDiffDeltas.end(),
               std::inserter(OurModified, OurModified.end()),
               [](SimplifiedDiffDelta const &SDD) {
                 return SDD.Type == SimplifiedDiffDelta::DeltaType::MODIFIED;
               });
  std::unordered_set<SimplifiedDiffDelta> TheirModified;
  std::copy_if(TheirDiffDeltas.begin(), TheirDiffDeltas.end(),
               std::inserter(TheirModified, TheirModified.end()),
               [](SimplifiedDiffDelta const &SDD) {
                 return SDD.Type == SimplifiedDiffDelta::DeltaType::MODIFIED;
               });
  OurDiffDeltas.insert(TheirModified.begin(), TheirModified.end());
  TheirDiffDeltas.insert(OurModified.begin(), OurModified.end());
  FileSet.OurDiffDeltas = std::vector<SimplifiedDiffDelta>(
      OurDiffDeltas.begin(), OurDiffDeltas.end());
  FileSet.TheirDiffDeltas = std::vector<SimplifiedDiffDelta>(
      TheirDiffDeltas.begin(), TheirDiffDeltas.end());

  std::unordered_set<SimplifiedDiffDelta> Union = OurDiffDeltas;
  for (SimplifiedDiffDelta const &SDD : TheirDiffDeltas) {
    if (Union.count(SDD) == 0) {
      Union.insert(SDD);
    }
  }
  FileSet.BaseDiffDeltas =
      std::vector<SimplifiedDiffDelta>(Union.begin(), Union.end());

  std::unordered_set<SimplifiedDiffDelta> Intersection;
  std::unordered_set<SimplifiedDiffDelta> &Smaller = OurDiffDeltas;
  std::unordered_set<SimplifiedDiffDelta> &Bigger = TheirDiffDeltas;
  if (Smaller.size() > Bigger.size()) {
    swap(Smaller, Bigger);
  }
  for (SimplifiedDiffDelta const &SDD : Smaller) {
    if (Bigger.count(SDD) > 0) {
      Intersection.insert(SDD);
    }
  }
  FileSet.BothModifiedDiffDeltas = std::vector<SimplifiedDiffDelta>(
      Intersection.begin(), Intersection.end());
}

void SourceFileCollector::collectIncludedFiles(
    Side S, const std::vector<SimplifiedDiffDelta> &DiffDeltas,
    const std::shared_ptr<clang::tooling::CompilationDatabase> &Compilations) {}
} // namespace sa
} // namespace mergebot
