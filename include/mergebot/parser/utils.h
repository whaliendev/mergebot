//
// Created by whalien on 04/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_PARSER_PARSER_UTILITY_H
#define MB_INCLUDE_MERGEBOT_PARSER_PARSER_UTILITY_H

#include <string>
#include <utility>

#include "mergebot/parser/node.h"

namespace mergebot {
namespace ts {

std::pair<size_t, std::string> getTranslationUnitComment(const ts::Node &root);

std::string getNodeComment(const ts::Node &node);

size_t beforeFirstChildEOLs(const ts::Node &node);

int nextSiblingDistance(const ts::Node &node, bool named = false);

/// \brief get the header guard of a header file
/// \param TUNode translation unit node
/// \param HeaderGuardStart child index to start match
/// \return std::pair of bool and header guard string, bool indicates whether
/// the header guard is a tradition guard
std::pair<bool, std::vector<std::string>> getHeaderGuard(
    const ts::Node &TUNode, size_t &BeforeBodyChildCnt, ts::Node &TURoot);

std::vector<std::pair<ts::Point, std::string>> getFrontDecls(
    const ts::Node &TURoot, size_t &cnt);

std::pair<bool, std::string> getComment(const ts::Node &commentNode,
                                        size_t &commentCnt);

bool isTypeDecl(const ts::Node &node);

}  // namespace ts
}  // namespace mergebot

#endif  // MB_INCLUDE_MERGEBOT_PARSER_PARSER_UTILITY_H
