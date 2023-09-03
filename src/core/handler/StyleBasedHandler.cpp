//
// Created by whalien on 13/03/23.
//

#include "mergebot/core/handler/StyleBasedHandler.h"
#include "mergebot/core/magic_enum_customization.h"
#include "mergebot/core/model/ConflictBlock.h"
#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/core/model/ConflictMark.h"
#include "mergebot/core/sa_utility.h"
#include "mergebot/filesystem.h"
#include "mergebot/server/vo/ResolutionResultVO.h"
#include "mergebot/utils/stringop.h"

#include <algorithm>
#include <clang/Format/Format.h>
#include <clang/Tooling/Core/Replacement.h>
#include <filesystem>
#include <magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <vector>

namespace mergebot {
namespace sa {

bool StyleBasedHandler::NeedFormat = true;
/// llvm, chromium, mozilla, google, webkit, gnu, microsoft
std::string StyleBasedHandler::Style = "google";
/// "ours" or "theirs"
std::string StyleBasedHandler::WhichSide = "ours";

void StyleBasedHandler::resolveConflictFiles(
    std::vector<ConflictFile> &ConflictFiles) {
  assert(ConflictFiles.size() &&
         "ConflictFile sizes should be greater than zero");

  spdlog::info("Resolving conflicts using style based handler...");

  bool WithBase = false;
  if (ConflictFiles[0].ConflictBlocks.size() != 0) {
    std::string_view FirstCR = ConflictFiles[0].ConflictBlocks[0].ConflictRange;
    size_t OurPos = FirstCR.find(magic_enum::enum_name(ConflictMark::OURS));
    size_t BasePos = FirstCR.find(magic_enum::enum_name(ConflictMark::BASE));
    size_t TheirPos = FirstCR.find(magic_enum::enum_name(ConflictMark::THEIRS));
    size_t EndPos = std::string_view::npos;
    WithBase = OurPos != EndPos && BasePos != EndPos && TheirPos != EndPos;
  }

  // check resolved, check not resolved
  for (ConflictFile &CF : ConflictFiles) {
    bool EverResolved = false, AllResolved = true;
    spdlog::debug("resolving {}...", CF.Filename);

    std::vector<server::BlockResolutionResult> ResolvedBlocks;
    for (ConflictBlock &CB : CF.ConflictBlocks) {
      std::string_view OurCode, TheirCode;
      if (WithBase) {
        OurCode = extractCodeFromConflictRange(
            CB.ConflictRange, magic_enum::enum_name(ConflictMark::OURS),
            magic_enum::enum_name(ConflictMark::BASE));
        TheirCode = extractCodeFromConflictRange(
            CB.ConflictRange, magic_enum::enum_name(ConflictMark::THEIRS),
            magic_enum::enum_name(ConflictMark::END));
      } else {
        OurCode = extractCodeFromConflictRange(
            CB.ConflictRange, magic_enum::enum_name(ConflictMark::OURS),
            magic_enum::enum_name(ConflictMark::THEIRS));
        TheirCode = extractCodeFromConflictRange(
            CB.ConflictRange, magic_enum::enum_name(ConflictMark::THEIRS),
            magic_enum::enum_name(ConflictMark::END));
      }
      assert((!OurCode.empty() || !TheirCode.empty()) &&
             "at least one side of code should not be empty");

      std::string DeflatedOurs =
          util::removeCommentsAndSpaces(std::string(OurCode));
      std::string DeflatedTheirs =
          util::removeCommentsAndSpaces(std::string(TheirCode));
      if (DeflatedOurs == DeflatedTheirs) { // style related conflicts
        spdlog::debug("deflated ours  : {}", DeflatedOurs);
        spdlog::debug("deflated theirs: {}", DeflatedTheirs);
        spdlog::info("ConflictBlock {} in File [{}] of Project [{}] is a "
                     "style related conflict",
                     CB.Index, CF.Filename, Meta.Project);
        std::string ResultStr = std::string(OurCode);
        if (NeedFormat) {
          std::string RefFile = findRefFile(CF.Filename);
          std::string FormattedCode = WhichSide == "ours"
                                          ? formatOneSide(OurCode, RefFile)
                                          : formatOneSide(TheirCode, RefFile);
          if (!FormattedCode.empty()) {
            ResultStr = FormattedCode;
          }
        }
        CB.Resolved = true;
        ResolvedBlocks.push_back({
            .index = CB.Index,
            .desc = "格式问题引发的合并冲突",
            .code = std::move(ResultStr),
        });
      }
    }

    std::for_each(CF.ConflictBlocks.begin(), CF.ConflictBlocks.end(),
                  [&](ConflictBlock const &CB) {
                    if (CB.Resolved) {
                      EverResolved = true;
                    } else {
                      AllResolved = false;
                    }
                  });
    // update resolved flag of ConflictFile
    CF.Resolved = AllResolved;
    // check any of blocks is resolved, if true, marshal resolution results to
    // MSCache dir; if false, do nothing
    if (EverResolved) {
      const std::string RelativePath =
          fs::relative(CF.Filename, Meta.ProjectPath).string();
      fs::path ResolutionDest =
          fs::path(Meta.MSCacheDir) / "resolutions" / pathToName(RelativePath);
      marshalResolutionResult(ResolutionDest.string(), RelativePath,
                              ResolvedBlocks);
    }

    // tidy up conflict files and their conflict blocks
    if (EverResolved) {
      tidyUpConflictFiles(ConflictFiles);
    }
  }
}

std::string StyleBasedHandler::formatOneSide(std::string_view SV,
                                             std::string const &RefFile) const {
  using namespace clang;
  format::FormatStyle FS;
  bool Predefined = format::getPredefinedStyle(
      Style, format::FormatStyle::LanguageKind::LK_Cpp, &FS);
  if (!Predefined) {
    spdlog::error("the style predefined[{}] is currently not supported by mbsa",
                  Style);
    return "";
  }
  unsigned Length = std::count(SV.begin(), SV.end(), '\n');
  bool IncompleteFormat = false;
  tooling::Replacements Replaces =
      format::reformat(/*Style=*/FS,
                       /*Code=*/SV,
                       /*Ranges=*/{{0, Length}},
                       /*FileName=*/RefFile,
                       /*IncompleteFormat=*/&IncompleteFormat);
  if (auto FormattedOrErr = tooling::applyAllReplacements(SV, Replaces)) {
    std::string &FormattedCode = *FormattedOrErr;
    spdlog::debug("formatted code in file {} is \n{}", RefFile, FormattedCode);
    return FormattedCode;
  } else {
    spdlog::warn(
        "we fail to formatted code, we'll directly recommend one side code");
    return "";
  }
}

std::string
StyleBasedHandler::findRefFile(std::string_view const &ConflictFilePath) const {
  assert(ConflictFilePath.find(Meta.ProjectPath) != std::string::npos &&
         "ConflictFilePath should starts with ProjectPath");
  assert(WhichSide == "ours" || WhichSide == "theirs");
  fs::path ProjectPath = fs::path(Meta.ProjectPath);
  if (!ProjectPath.has_filename()) {
    ProjectPath = ProjectPath / ""; // add a trailing path separator
  }
  std::string ProjectPathStr = ProjectPath.string();
  std::string_view ConflictFile =
      ConflictFilePath.substr(ProjectPathStr.length());
  return (fs::path(Meta.MSCacheDir) / WhichSide / ConflictFile).string();
}
} // namespace sa
} // namespace mergebot
