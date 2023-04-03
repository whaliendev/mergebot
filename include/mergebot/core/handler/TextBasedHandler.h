//
// Created by whalien on 13/03/23.
//

#ifndef MB_TEXTBASEDHANDLER_H
#define MB_TEXTBASEDHANDLER_H

#include "mergebot/core/handler/SAHandler.h"

namespace mergebot {
namespace sa {

class TextBasedHandler : public SAHandler {
public:
  TextBasedHandler(ProjectMeta Meta, std::string Name = __FILE_NAME__)
      : SAHandler(Meta, Name) {}

private:
  void resolveConflictFiles(
      std::vector<ConflictFile> &ConflictFiles) const override {}
};

} // namespace sa
} // namespace mergebot

#endif // MB_TEXTBASEDHANDLER_H
