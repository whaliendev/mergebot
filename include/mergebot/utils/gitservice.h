//
// Created by whalien on 04/05/23.
//

#ifndef MB_GITSERVICE_H
#define MB_GITSERVICE_H
#include <unordered_set>

#include "mergebot/core/model/SimplifiedDiffDelta.h"
namespace mergebot {
namespace util {
bool isCppFile(std::string_view path);

std::unordered_set<sa::SimplifiedDiffDelta> list_cpp_diff_files(
    std::string_view repo_path, std::string_view old_commit,
    std::string_view new_commit);
}  // namespace util
}  // namespace mergebot

#endif  // MB_GITSERVICE_H
