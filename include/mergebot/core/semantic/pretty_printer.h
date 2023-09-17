//
// Created by whalien on 30/04/23.
//

#ifndef MB_PRETTYPRINTER_H
#define MB_PRETTYPRINTER_H

#include "mergebot/core/model/SemanticNode.h"
#include <memory>
namespace mergebot::sa {
std::string PrettyPrintTU(const std::shared_ptr<SemanticNode> &TUNode,
                          const std::string &DestDir);
} // namespace mergebot::sa

#endif // MB_PRETTYPRINTER_H
