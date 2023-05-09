//
// Created by whalien on 13/03/23.
//

#ifndef MB_TEXTBASEDHANDLER_H
#define MB_TEXTBASEDHANDLER_H

#include "mergebot/core/handler/SAHandler.h"
#include "mergebot/server/vo/ResolutionResultVO.h"

namespace mergebot {
namespace sa {

class TextBasedHandler : public SAHandler {
public:
  explicit TextBasedHandler(ProjectMeta Meta, std::string Name = __FILE_NAME__)
      : SAHandler(Meta, Name) {}

private:
  void resolveConflictFiles(std::vector<ConflictFile> &ConflictFiles) override;
  bool checkOneSideDelta(std::string_view Our, std::string_view Their,
                         ConflictFile const &CF,
                         server::BlockResolutionResult &BRR);
  bool checkInclusion(std::string_view Our, std::string_view Their,
                      ConflictFile const &CF,
                      server::BlockResolutionResult &BRR);
  bool doListMerge(std::string_view Our, std::string_view Their,
                   ConflictFile const &CF, server::BlockResolutionResult &BRR);
};

} // namespace sa
} // namespace mergebot

#endif // MB_TEXTBASEDHANDLER_H
