//
// Created by whalien on 16/02/23.
//

#ifndef MB_SA_UTILITY_H
#define MB_SA_UTILITY_H
#include <llvm/Support/Error.h>
#include <llvm/Support/ErrorOr.h>
#include <spdlog/spdlog.h>

#include <shared_mutex>
#include <string>
#include <string_view>
#include <vector>

#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/server/vo/ResolutionResultVO.h"

namespace mergebot {
template <typename T> void hash_combine(size_t &seed, const T &v) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename T> struct VectorHash {
  std::size_t operator()(const std::vector<T> &vec) const {
    std::size_t seed = 0;
    for (const auto &elem : vec) {
      hash_combine(seed, elem);
    }
    return seed;
  }
};

namespace utils {
static std::shared_mutex rwMutex;
static std::mutex peekMutex;

// git diff --name-only --diff-filter=U
llvm::ErrorOr<std::string> ExecCommand(std::string_view sv, int timeout = 10,
                                       int exitCode = 0,
                                       bool checkExitCode = true);

template <typename InputIt1, typename InputIt2, typename Compare>
typename std::enable_if<
    std::is_same<typename std::iterator_traits<InputIt1>::iterator_category,
                 std::random_access_iterator_tag>::value &&
        std::is_same<typename std::iterator_traits<InputIt2>::iterator_category,
                     std::random_access_iterator_tag>::value,
    bool>::type
hasSameElements(InputIt1 first1, InputIt1 last1, InputIt2 first2,
                InputIt2 last2, Compare comp);

std::pair<int, struct flock> lockRDFD(std::string_view path);
std::pair<int, struct flock> lockWRFD(std::string_view path);
bool unlockFD(std::string_view path, int fd, struct flock &lck);

template <typename Func, typename... Args>
static auto MeasureRunningTime(Func &&func, Args &&...args) {
  // Check if the function is invocable with the given arguments
  static_assert(std::is_invocable_v<Func, Args...>,
                "Func is not invocable with the provided arguments");

  using FuncReturnType = std::invoke_result_t<Func, Args...>;

  auto start = std::chrono::steady_clock::now();
  if constexpr (std::is_same_v<FuncReturnType, void>) {
    std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
    auto end = std::chrono::steady_clock::now();
    long elapsed_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
            .count();
    return elapsed_time;
  } else {
    FuncReturnType result =
        std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
    auto end = std::chrono::steady_clock::now();
    long elapsed_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
            .count();
    return std::tuple<long, FuncReturnType>(elapsed_time, result);
  }
}

} // namespace utils

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

std::string nameToPath(const std::string &name);

/// extract code between two markers in conflict code range, note that the
/// marker line will be discarded from code
/// !!! we have to make sure the lifetime of Source's referenced object should
/// be longer than the returned value.
/// \param Source the source string_view of conflict code
/// \param StartMark the start marker
/// \param EndMarker the end marker
/// \return code between two markers
std::string_view extractCodeFromConflictRange(std::string_view Source,
                                              std::string_view StartMark,
                                              std::string_view EndMarker);

std::string trim(std::string const &code);

std::string getUnqualifiedName(std::string_view name);

std::string getFullQualifiedName(const std::string &QualifiedName,
                                 std::string &&Unqualified);
} // namespace sa
} // namespace mergebot

#endif // MB_SA_UTILITY_H
