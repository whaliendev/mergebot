//
// Created by whalien on 30/04/23.
//

#ifndef MB_PRETTYPRINTER_H
#define MB_PRETTYPRINTER_H

#include "mergebot/core/model/SemanticNode.h"
#include "mergebot/filesystem.h"
#include <memory>
namespace mergebot::sa {
std::string PrintTU(const std::shared_ptr<SemanticNode> &TUNode,
                    const std::string &DestDir);

std::string PrettyPrintTU(const std::shared_ptr<SemanticNode> &TUNode,
                          const std::string &DestDir,
                          const std::string &ClangFormatPath =
                              (fs::current_path() / ".clang-format").string());

bool FormatSource(const std::string &DestFile,
                  const std::string &ClangFormatPath);
} // namespace mergebot::sa

#endif // MB_PRETTYPRINTER_H
