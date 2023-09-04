//
// Created by whalien on 04/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_PARSER_PARSER_UTILITY_H
#define MB_INCLUDE_MERGEBOT_PARSER_PARSER_UTILITY_H

#include <string>

#include "mergebot/parser/node.h"

namespace mergebot {
namespace ts {
std::string getTranslationUnitComment(const ts::Node &root);

std::string getNodeComment(const ts::Node &node);
}  // namespace ts
}  // namespace mergebot

#endif  // MB_INCLUDE_MERGEBOT_PARSER_PARSER_UTILITY_H
