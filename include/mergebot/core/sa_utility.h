//
// Created by whalien on 16/02/23.
//

#ifndef MB_SA_UTILITY_H
#define MB_SA_UTILITY_H
#include <llvm/Support/Error.h>

#include <vector>

#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/server/vo/ResolutionResultVO.h"
#include <llvm/Support/ErrorOr.h>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

namespace mergebot {
template <typename T> void hash_combine(size_t &seed, const T &v) {
  const size_t prime = 31;
  std::hash<T> hasher;
  seed = prime * seed + hasher(v);
}

namespace util {
// git diff --name-only --diff-filter=U
llvm::ErrorOr<std::string> ExecCommand(std::string_view sv, int timeout = 10,
                                       int exitCode = 0);

template <typename InputIt1, typename InputIt2, typename Compare>
typename std::enable_if<
    std::is_same<typename std::iterator_traits<InputIt1>::iterator_category,
                 std::random_access_iterator_tag>::value &&
        std::is_same<typename std::iterator_traits<InputIt2>::iterator_category,
                     std::random_access_iterator_tag>::value,
    bool>::type
hasSameElements(InputIt1 first1, InputIt1 last1, InputIt2 first2,
                InputIt2 last2, Compare comp);
} // namespace util

namespace sa {
void handleSAExecError(std::error_code err, std::string_view cmd);

[[deprecated(
    "Use constructConflictFiles() instead.")]] std::vector<ConflictFile>
extractConflictBlocks(std::vector<std::string> &ConflictFiles);
std::vector<ConflictFile>
constructConflictFiles(std::vector<std::string> &ConflictFilePaths);

void marshalResolutionResult(
    std::string_view DestPath, std::string_view FileName,
    std::vector<server::BlockResolutionResult> const &Results);

void tidyUpConflictFiles(std::vector<ConflictFile> &ConflictFiles);

std::string pathToName(std::string_view path);

std::string nameToPath(std::string_view name);
} // namespace sa
} // namespace mergebot

#endif // MB_SA_UTILITY_H
