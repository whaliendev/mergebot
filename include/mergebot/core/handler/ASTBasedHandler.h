//
// Created by whalien on 13/03/23.
//

#ifndef MB_ASTBASEDHANDLER_H
#define MB_ASTBASEDHANDLER_H

#include "SAHandler.h"
#include <string>
#include <vector>

namespace mergebot {
namespace sa {
class ASTBasedHandler : public SAHandler {
public:
  ASTBasedHandler(ProjectMeta Meta, std::string Name = __FILE_NAME__)
      : SAHandler(Meta, Name) {}

private:
  void resolveConflictFiles(
      std::vector<ConflictFile> &ConflictFiles) const override {}
};
} // namespace sa
} // namespace mergebot

#endif // MB_ASTBASEDHANDLER_H
