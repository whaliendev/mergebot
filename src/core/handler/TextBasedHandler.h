//
// Created by whalien on 13/03/23.
//

#ifndef MB_TEXTBASEDHANDLER_H
#define MB_TEXTBASEDHANDLER_H

#include "SAHandler.h"

namespace mergebot {
namespace sa {

class TextBasedHandler : public SAHandler {
public:
  TextBasedHandler(std::string Name = __FILE_NAME__) : SAHandler(Name) {}

private:
  void resolveConflictFiles(
      std::vector<ConflictFile> &ConflictFiles) const override {}
};

} // namespace sa
} // namespace mergebot

#endif // MB_TEXTBASEDHANDLER_H
