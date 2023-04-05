//
// Created by whalien on 13/03/23.
//

#include "mergebot/core/handler/StyleBasedHandler.h"
#include "mergebot/core/model/ConflictBlock.h"
#include "mergebot/core/model/ConflictMark.h"
#include "mergebot/filesystem.h"
#include "mergebot/magic_enum_customization.h"
#include "mergebot/server/vo/BlockResolutionResult.h"
#include "mergebot/utils/stringop.h"

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

namespace _details {
/// extract code between two markers in conflict code range, note that the
/// marker line will be discarded from code
/// !!! we have to make sure the lifetime of Source's referenced object should
/// be longer than the returned value.
/// \param Source the source string_view of conflict code
/// \param StartMark the start marker
/// \param EndMarker the end marker
/// \return code between two markers
std::string_view extractCodeFromConflictRange(std::string_view Source,
                                              std::string_view StartMark,
                                              std::string_view EndMarker) {
  size_t StartPos = Source.find(StartMark);
  while (StartPos != std::string_view::npos && StartPos != Source.length() &&
         Source[StartPos++] != '\n')
    ;
  assert(StartPos != std::string_view::npos && StartPos != Source.length() &&
         "illegal conflict range, start marker line is in bad format");
  size_t EndPos = Source.find(EndMarker, StartPos);
  assert(EndPos != std::string_view::npos &&
         "illegal conflict range, no end marker");
  if (StartPos == std::string_view::npos || StartPos == Source.length() ||
      EndPos == std::string_view::npos) {
    return std::string_view();
  }
  return Source.substr(StartPos, EndPos - StartPos);
}
} // namespace _details

bool StyleBasedHandler::NeedFormat = true;
/// llvm, chromium, mozilla, google, webkit, gnu, microsoft
std::string StyleBasedHandler::Style = "google";
/// "ours" or "theirs"
std::string StyleBasedHandler::WhichSide = "ours";

void StyleBasedHandler::resolveConflictFiles(
    std::vector<ConflictFile> &ConflictFiles) const {
  assert(ConflictFiles.size() &&
         "ConflictFile sizes should be greater than zero");

  spdlog::info("we are resolving conflicts use style based handler");

  std::string_view FirstCR = ConflictFiles[0].ConflictBlocks[0].ConflictRange;
  size_t OurPos = FirstCR.find(magic_enum::enum_name(ConflictMark::OURS));
  size_t BasePos = FirstCR.find(magic_enum::enum_name(ConflictMark::BASE));
  size_t TheirPos = FirstCR.find(magic_enum::enum_name(ConflictMark::THEIRS));
  size_t EndPos = std::string_view::npos;
  bool WithBase = OurPos != EndPos && BasePos != EndPos && TheirPos != EndPos;

  for (ConflictFile &CF : ConflictFiles) {
    if (CF.Resolved) {
      continue;
    }
    spdlog::debug("resolving {}...", CF.Filename);

    std::vector<server::BlockResolutionResult> ResolvedBlocks;

    for (ConflictBlock &CB : CF.ConflictBlocks) {
      if (CB.Resolved) {
        continue;
      }
      std::string_view OurCode, TheirCode;
      if (WithBase) {
        OurCode = _details::extractCodeFromConflictRange(
            CB.ConflictRange, magic_enum::enum_name(ConflictMark::OURS),
            magic_enum::enum_name(ConflictMark::BASE));
        TheirCode = _details::extractCodeFromConflictRange(
            CB.ConflictRange, magic_enum::enum_name(ConflictMark::THEIRS),
            magic_enum::enum_name(ConflictMark::END));
      } else {
        OurCode = _details::extractCodeFromConflictRange(
            CB.ConflictRange, magic_enum::enum_name(ConflictMark::OURS),
            magic_enum::enum_name(ConflictMark::THEIRS));
        TheirCode = _details::extractCodeFromConflictRange(
            CB.ConflictRange, magic_enum::enum_name(ConflictMark::THEIRS),
            magic_enum::enum_name(ConflictMark::END));
      }
      assert((!OurCode.empty() || !TheirCode.empty()) &&
             "at least one side of code should not be empty");

      std::string DeflatedOurs = util::removeSpaces(OurCode);
      std::string DeflatedTheirs = util::removeSpaces(TheirCode);
      if (DeflatedOurs == DeflatedTheirs) { // style related conflicts
        spdlog::info("ConflictBlock {} in File [{}] of Project [{}] is a "
                     "merge conflict caused by formatting issues",
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
        ResolvedBlocks.push_back({.index = CB.Index,
                                  .code = std::move(ResultStr),
                                  .desc = "格式问题引发的合并冲突"});
        spdlog::debug(ResolvedBlocks[0].code);
      }
    }
    // marshal resolution result to specific file, ResolvedBlocks
  }
  // delete all resolved conflict files and conflict blocks that are resolved
}

std::string StyleBasedHandler::formatOneSide(std::string_view sv,
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
  unsigned Length = std::count(sv.begin(), sv.end(), '\n');
  bool IncompleteFormat = false;
  tooling::Replacements Replaces =
      format::reformat(/*Style=*/FS,
                       /*Code=*/sv,
                       /*Ranges=*/{{0, Length}},
                       /*FileName=*/RefFile,
                       /*IncompleteFormat=*/&IncompleteFormat);
  if (auto FormattedOrErr = tooling::applyAllReplacements(sv, Replaces)) {
    std::string &FormattedCode = *FormattedOrErr;
    spdlog::debug("formatted code in file {} is {{}}", RefFile, FormattedCode);
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
