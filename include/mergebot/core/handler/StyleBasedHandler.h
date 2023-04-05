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
  StyleBasedHandler(ProjectMeta Meta, std::string Name = __FILE_NAME__)
      : SAHandler(Meta, Name) {}

private:
  void
  resolveConflictFiles(std::vector<ConflictFile> &ConflictFiles) const override;

  std::string findRefFile(std::string_view const &ConflictFilePath) const;

  std::string formatOneSide(std::string_view sv,
                            std::string const &RefFile) const;

  // settings related vars
  static bool NeedFormat;
  static std::string Style;
  static std::string WhichSide;
};

} // namespace sa
} // namespace mergebot

#endif // MB_STYLEBASEDHANDLER_H
