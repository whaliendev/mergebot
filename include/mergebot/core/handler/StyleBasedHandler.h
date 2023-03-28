//
// Created by whalien on 13/03/23.
//

#ifndef MB_STYLEBASEDHANDLER_H
#define MB_STYLEBASEDHANDLER_H

#include "mergebot/core/handler/SAHandler.h"

namespace mergebot {
namespace sa {

class StyleBasedHandler : public SAHandler {
public:
  StyleBasedHandler(std::string Name = __FILE_NAME__) : SAHandler(Name) {}

private:
  void resolveConflictFiles(
      std::vector<ConflictFile> &ConflictFiles) const override {}
};

} // namespace sa
} // namespace mergebot

#endif // MB_STYLEBASEDHANDLER_H
