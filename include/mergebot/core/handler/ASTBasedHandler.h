//
// Created by whalien on 13/03/23.
//

#ifndef MB_ASTBASEDHANDLER_H
#define MB_ASTBASEDHANDLER_H

#include "SAHandler.h"
#include "mergebot/core/model/ConflictFile.h"
#include <string>
#include <vector>

namespace mergebot {
namespace sa {
class ASTBasedHandler : public SAHandler {
public:
  explicit ASTBasedHandler(ProjectMeta Meta, std::string Name = __FILE_NAME__)
      : SAHandler(Meta, Name) {}

private:
  void
  resolveConflictFiles(std::vector<ConflictFile> &ConflictFiles) const override;

  std::vector<std::string>
  collectAnalysisFileSet(const std::vector<ConflictFile> &ConflictFiles,
                         std::string_view ProjectPath) const;

  bool replaceProjPath(std::string const &CompDBPath,
                       std::string_view ProjPath) const;

  const static std::string CompDBRelative;
};
} // namespace sa
} // namespace mergebot

#endif // MB_ASTBASEDHANDLER_H
