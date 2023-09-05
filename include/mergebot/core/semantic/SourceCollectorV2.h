//
// Created by whalien on 05/05/23.
//

#ifndef MB_SOURCECOLLECTORV2_H
#define MB_SOURCECOLLECTORV2_H

#include "mergebot/core/handler/SAHandler.h"
#include "mergebot/core/model/Side.h"
#include "mergebot/core/model/SimplifiedDiffDelta.h"
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace mergebot {
namespace sa {
class SourceCollectorV2 {
public:
  /**
   * @brief \p AnalysisSourceTuple stores the source sets to be analyzed in
   * ASTBasedHandler
   *
   * the fields can be classified into 3 pairs, xxxSourceList and
   * xxxDirectIncluded are a pair, xxxSourceList represents sources to be
   * analyzed in one revision, xxxDirectIncluded stores source's direct included
   * source
   */
  struct AnalysisSourceTuple {
    std::vector<std::string> OurSourceList;
    std::vector<std::string> BaseSourceList;
    std::vector<std::string> TheirSourceList;
    std::unordered_map<std::string, std::vector<std::string>> OurDirectIncluded;
    std::unordered_map<std::string, std::vector<std::string>>
        BaseDirectIncluded;
    std::unordered_map<std::string, std::vector<std::string>>
        TheirDirectIncluded;
  };

  /// default settings
  explicit SourceCollectorV2(ProjectMeta const &Meta) : Meta(Meta) {}
  /// only want to analysis both modified file sets
  SourceCollectorV2(ProjectMeta const &Meta, bool OnlyBothModified)
      : Meta(Meta), OnlyBothModified(OnlyBothModified), SourceTuple{} {}
  /// when we want to collect all the included files for context analysis
  SourceCollectorV2(ProjectMeta const &Meta,
                    std::shared_ptr<clang::tooling::CompilationDatabase> const
                        &OurCompilations,
                    std::shared_ptr<clang::tooling::CompilationDatabase> const
                        &BaseCompilations,
                    std::shared_ptr<clang::tooling::CompilationDatabase> const
                        &TheirCompilations)
      : Meta(Meta), LookupIncluded(true), OnlyBothModified(false),
        SourceTuple{}, OurCompilations(OurCompilations),
        BaseCompilations(BaseCompilations),
        TheirCompilations(TheirCompilations) {}

  /// collect analysis sources, we can set `LookupIncluded` before
  /// this operation to add more context info
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

  void extendIncludedSources(Side S);

  ProjectMeta Meta;
  bool LookupIncluded = false;
  // only textual conflicts, don't consider file level
  bool OnlyBothModified = false;
  AnalysisSourceTuple SourceTuple;
  std::shared_ptr<clang::tooling::CompilationDatabase> OurCompilations;
  std::shared_ptr<clang::tooling::CompilationDatabase> BaseCompilations;
  std::shared_ptr<clang::tooling::CompilationDatabase> TheirCompilations;
};

class IncludeLookupCallback : public clang::PPCallbacks {
public:
  void LexedFileChanged(clang::FileID FID, LexedFileChangeReason Reason,
                        clang::SrcMgr::CharacteristicKind FileType,
                        clang::FileID PrevFID,
                        clang::SourceLocation Loc) override;
  void InclusionDirective(clang::SourceLocation HashLoc,
                          const clang::Token &IncludeTok,
                          llvm::StringRef FileName, bool IsAngled,
                          clang::CharSourceRange FilenameRange,
                          clang::OptionalFileEntryRef File,
                          llvm::StringRef SearchPath,
                          llvm::StringRef RelativePath,
                          const clang::Module *Imported,
                          clang::SrcMgr::CharacteristicKind FileType) override;

  std::unordered_set<std::string> directIncludes() const {
    return DirectIncludes;
  }

private:
  uint Depth = 0;
  std::unordered_set<std::string> DirectIncludes;
};

class IncludeLookupActionFactory
    : public clang::tooling::FrontendActionFactory {
public:
  std::vector<std::string> copyIncludedFiles() { return IncludedFiles; }
  std::unique_ptr<clang::FrontendAction> create() override {
    class IncludeLookupAction : public clang::PreprocessOnlyAction {
    public:
    protected:
      bool BeginSourceFileAction(clang::CompilerInstance &CI) override {
        std::unique_ptr<IncludeLookupCallback> Callback =
            std::make_unique<IncludeLookupCallback>();
        clang::Preprocessor &PP = CI.getPreprocessor();
        PP.addPPCallbacks(std::move(Callback));
        return true;
      }
      void EndSourceFileAction() override {
        clang::CompilerInstance &CI = getCompilerInstance();
        clang::Preprocessor &PP = CI.getPreprocessor();
        IncludeLookupCallback *Callback =
            static_cast<IncludeLookupCallback *>(PP.getPPCallbacks());
        std::unordered_set<std::string> DirectIncludes =
            Callback->directIncludes();
        IncludedFiles = std::vector<std::string>(DirectIncludes.begin(),
                                                 DirectIncludes.end());
      }
    };
    return std::make_unique<IncludeLookupAction>();
  }

private:
  static std::vector<std::string> IncludedFiles;
};

} // namespace sa
} // namespace mergebot

#endif // MB_SOURCECOLLECTORV2_H
