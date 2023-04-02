//
// Created by whalien on 13/03/23.
//

#include "mergebot/core/handler/StyleBasedHandler.h"

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
  while (StartPos != Source.length() && Source[StartPos++] != '\n')
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
  // -1 means remove the last new line
  return Source.substr(StartPos, EndPos - StartPos - 1);
}
} // namespace _details

void StyleBasedHandler::resolveConflictFiles(
    std::vector<ConflictFile> &ConflictFiles) const {
  assert(ConflictFiles.size() &&
         "ConflictFile sizes should be greater than zero");
  spdlog::info("we are resolving conflicts use style based handler");
  for (ConflictFile &CF : ConflictFiles) {
    spdlog::debug("resolve {}...", CF.Filename);
  }
}
} // namespace sa
} // namespace mergebot
