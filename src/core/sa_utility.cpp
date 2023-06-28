//
// Created by whalien on 21/02/23.
//
#include <fmt/format.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/ErrorOr.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <spdlog/spdlog.h>
#include <sys/wait.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <magic_enum.hpp>
#include <memory>
#include <shared_mutex>
#include <stdexcept>
#include <utility>
#include <vector>

#include "mergebot/core/magic_enum_customization.h"
#include "mergebot/core/model/ConflictBlock.h"
#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/core/model/ConflictMark.h"
#include "mergebot/core/sa_utility.h"
#include "mergebot/utils/fileio.h"
#include "mergebot/utils/format.h"
#include "mergebot/utils/pathop.h"
#include "mergebot/utils/stringop.h"

namespace mergebot {
namespace utils {
// git diff --name-only --diff-filter=U
/// Linux specific command execution encapsulation
/// \param cmd command to execute
/// \param timeout timout to execute the command, the unit is second.
/// \return `llvm::Error<std::string>`, an wrapper for std::error_code or the
/// output of `popen` returned stream, with the last new line removed
[[nodiscard]] llvm::ErrorOr<std::string>
ExecCommand(std::string_view sv, int timeout, int exitCode) {
  std::string result;

  std::array<char, 128> buffer;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(sv.data(), "r"), pclose);
  if (!pipe) {
    return std::make_error_code(std::errc::io_error);
  }

  int status = 0;
  auto start_time = std::chrono::steady_clock::now();
  while (true) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed_time =
        std::chrono::duration_cast<std::chrono::seconds>(now - start_time)
            .count();
    if (elapsed_time >= timeout) {
      pipe.reset();
      return std::make_error_code(std::errc::timed_out);
    }

    int res = waitpid(0, &status, WNOHANG);
    if (res == -1) { // exit accidentally
      return std::error_code(errno, std::generic_category());
    } else {                   // exit intentionally
      if (WIFEXITED(status)) { // subprocess exit
        if (WEXITSTATUS(status) != 0 &&
            WEXITSTATUS(status) != exitCode) { // exit accidentally
          spdlog::info("exit status: {}", WEXITSTATUS(status));
          return std::error_code(status, std::generic_category());
        }

        // exit successfully
        while (fgets(buffer.data(), buffer.size(), pipe.get())) {
          result += buffer.data();
        }
        if (result.length() && result[result.length() - 1])
          result.pop_back();
        return result;
      } else if (WIFSIGNALED(status)) { // interrupted by signal
        return std::make_error_code(std::errc::interrupted);
      } else { // res equals to pid id
      }
    }
    // sleep for a second to prevent high cpu occupation
    usleep(1000);
  }
}

template <typename InputIt1, typename InputIt2, typename Compare>
typename std::enable_if<
    std::is_same<typename std::iterator_traits<InputIt1>::iterator_category,
                 std::random_access_iterator_tag>::value &&
        std::is_same<typename std::iterator_traits<InputIt2>::iterator_category,
                     std::random_access_iterator_tag>::value,
    bool>::type
hasSameElements(InputIt1 first1, InputIt1 last1, InputIt2 first2,
                InputIt2 last2, Compare comp) {
  static_assert(
      std::is_invocable_r<
          bool, Compare, typename std::iterator_traits<InputIt1>::value_type,
          typename std::iterator_traits<InputIt2>::value_type>::value,
      "Compare should be invocable with two arguments of the value type");

  for (InputIt1 it1 = first1; it1 != last1; ++it1) {
    if (std::find_if(first2, last2, [&](const auto &val) {
          return comp(*it1, val);
        }) != last2) {
      return true;
    }
  }
  return false;
}

std::pair<int, struct flock> lockRDFD(std::string_view path) {
  std::shared_lock<std::shared_mutex> lock(rwMutex);
  int fd = -1;
  struct flock fileLck;

  fd = open(path.data(), O_RDONLY);
  if (fd == -1) {
    spdlog::error("fail to open file [{}] for read, reason: {}", path,
                  strerror(errno));
    return std::make_pair(-1, fileLck);
  }

  fileLck = {.l_type = F_RDLCK, .l_whence = SEEK_SET, .l_start = 0, .l_len = 0};
  if (fcntl(fd, F_SETLKW, &fileLck) == -1) {
    spdlog::error("fail to acquire read lock for file [{}]", path);
    close(fd);
    return std::make_pair(-1, fileLck);
  }
  return std::make_pair(fd, fileLck);
}

std::pair<int, struct flock> lockWRFD(std::string_view path) {
  std::lock_guard<std::shared_mutex> lock(rwMutex);
  int fd = -1;
  struct flock fileLck;

  // open for write
  // didn't exist, create one
  // exist, trunc
  fd = open(path.data(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd == -1) {
    spdlog::error("fail to open file [{}] for write, reason: {}", path,
                  strerror(errno));
    return std::make_pair(-1, fileLck);
  }

  fileLck = {.l_type = F_WRLCK, .l_whence = SEEK_SET, .l_start = 0, .l_len = 0};
  if (fcntl(fd, F_SETLKW, &fileLck) == -1) {
    spdlog::error("fail to acquire write lock for file [{}]", path);
    close(fd);
    return std::make_pair(-1, fileLck);
  }
  return std::make_pair(fd, fileLck);
}

bool unlockFD(std::string_view path, int fd, struct flock &lck) {
  std::lock_guard<std::shared_mutex> lock(rwMutex);
  lck.l_type = F_UNLCK;
  if (fcntl(fd, F_SETLKW, &lck) == -1) {
    spdlog::error("fail to release lock for file {}", path);
    return false;
  }
  return true;
}
} // namespace utils

namespace sa {
namespace _details {
std::vector<ConflictBlock>
constructConflictFile(std::unique_ptr<llvm::MemoryBuffer> &File) {
  int Index = 0;
  std::vector<ConflictBlock> ConflictBlocks;
  const char *Start = File->getBufferStart();
  const char *End = File->getBufferEnd();
  for (const char *Pos = Start, *LineStart = Start, *BlockStart = Start;
       Pos != End; LineStart = ++Pos) {
    Pos = std::find(Pos, End, '\n');
    std::string_view Line(LineStart, Pos - LineStart);
    if (Line.size() < 7)
      continue;
    std::string_view LineMark = Line.substr(0, 7);
    if (LineMark == magic_enum::enum_name(ConflictMark::OURS)) {
      BlockStart = LineStart;
      ConflictBlock Block;
      Block.Index = ++Index;
      while (Pos != End) {
        Pos = std::find(Pos, End, '\n');
        Line = std::string_view(LineStart, Pos - LineStart);
        if (Line.size() < 7) {
          LineStart = ++Pos;
          continue;
        }
        LineMark = Line.substr(0, 7);
        if (LineMark == magic_enum::enum_name(ConflictMark::END)) {
          Block.ConflictRange = std::string(BlockStart, Pos - BlockStart + 1);
          ConflictBlocks.push_back(std::move(Block));
          break;
        }
        LineStart = ++Pos;
      }
    }
  }
  return ConflictBlocks;
}

void tidyUpConflictBlocks(std::vector<ConflictBlock> &ConflictBlocks) {
  bool Shrunk =
      std::any_of(ConflictBlocks.begin(), ConflictBlocks.end(),
                  [](ConflictBlock const &CB) { return CB.Resolved; });
  if (Shrunk) {
    std::vector<ConflictBlock> NewConflictBlocks;
    NewConflictBlocks.reserve(ConflictBlocks.size());
    std::for_each(ConflictBlocks.begin(), ConflictBlocks.end(),
                  [&](ConflictBlock &CB) {
                    if (!CB.Resolved) {
                      NewConflictBlocks.push_back(std::move(CB));
                    }
                  });
    ConflictBlocks = NewConflictBlocks;
  }
}

// 判断一行是否为空行
bool isBlankLine(const std::string &line) {
  for (const char &c : line) {
    if (!isspace(c))
      return false;
  }
  return true;
}

// 去除一行的注释
std::string removeComment(const std::string &line) {
  std::string result;
  bool inSingleQuote = false;
  bool inDoubleQuote = false;
  for (size_t i = 0; i < line.size(); i++) {
    char c = line[i];
    if (c == '\'') {
      inSingleQuote = !inSingleQuote;
      result += c;
    } else if (c == '\"') {
      inDoubleQuote = !inDoubleQuote;
      result += c;
    } else if (inSingleQuote || inDoubleQuote) {
      result += c;
    } else {
      if (i < line.size() - 1 && c == '/' && line[i + 1] == '/') {
        break;
      } else {
        result += c;
      }
    }
  }
  return result;
}
} // namespace _details

void handleSAExecError(std::error_code err, std::string_view cmd) {
  if (err == std::errc::timed_out) {
    spdlog::warn(mergebot::util::format("timeout to executing {}", cmd));
  } else if (err == std::errc::interrupted) {
    spdlog::error(
        mergebot::util::format("cmd [{}] accidentally interrupted", cmd));
  } else if (err == std::errc::io_error) {
    spdlog::error(mergebot::util::format(
        "open popen or waitpid failed for cmd [{}]", cmd));
  } else {
    spdlog::error(mergebot::util::format(
        "cmd [{}] accidentally exited with exit code {}", cmd, err.value()));
  }
}

std::vector<ConflictFile>
constructConflictFiles(std::vector<std::string> &ConflictFilePaths) {
  using namespace llvm;
  std::vector<ConflictFile> ConflictFiles;
  ConflictFiles.reserve(ConflictFilePaths.size());
  for (const auto &ConflictFilePath : ConflictFilePaths) {
    std::string AbsoluteFilePath = util::toabs(ConflictFilePath);
    ErrorOr<std::unique_ptr<MemoryBuffer>> FileOrErr =
        MemoryBuffer::getFile(AbsoluteFilePath);
    if (auto Err = FileOrErr.getError()) {
      spdlog::error(
          "failed to extract conflict blocks for source file [{}], err "
          "message: {}",
          AbsoluteFilePath, Err.message());
      continue;
    }
    std::unique_ptr<MemoryBuffer> File = std::move(FileOrErr.get());
    std::vector<ConflictBlock> ConflictBlocks =
        _details::constructConflictFile(File);
    ConflictFiles.emplace_back(std::move(AbsoluteFilePath),
                               std::move(ConflictBlocks));
  }
  return ConflictFiles;
}

void tidyUpConflictFiles(std::vector<ConflictFile> &ConflictFiles) {
  bool Shrunk =
      std::any_of(ConflictFiles.begin(), ConflictFiles.end(),
                  [&](ConflictFile const &CF) { return CF.Resolved; });
  if (Shrunk) {
    std::vector<ConflictFile> NewConflictFiles;
    NewConflictFiles.reserve(ConflictFiles.size());
    std::for_each(ConflictFiles.begin(), ConflictFiles.end(),
                  [&](ConflictFile &CF) {
                    if (!CF.Resolved) {
                      _details::tidyUpConflictBlocks(CF.ConflictBlocks);
                      NewConflictFiles.push_back(std::move(CF));
                    }
                  });
    ConflictFiles = NewConflictFiles;
  } else { // all conflict files are reserved, we have to tidy up their
           // conflict blocks
    std::for_each(ConflictFiles.begin(), ConflictFiles.end(),
                  [&](ConflictFile &CF) {
                    _details::tidyUpConflictBlocks(CF.ConflictBlocks);
                  });
  }
}

void marshalResolutionResult(
    std::string_view DestPath, std::string_view FileName,
    std::vector<server::BlockResolutionResult> const &Results) {
  fs::path ResolutionFilePath = fs::path(DestPath);
  std::ifstream ResolutionFS(ResolutionFilePath.string());
  // file not exists or do exist but is illegal, marshal directly
  // FIXME(hwa): when two MS API requests occur simultaneously,
  // there may be a data race
  if (!fs::exists(ResolutionFilePath) ||
      (fs::exists(ResolutionFilePath) &&
       !nlohmann::json::accept(ResolutionFS))) {
    server::FileResolutionResult FR{.filepath = std::string(FileName),
                                    .resolutions = Results};
    nlohmann::json JsonToDump = FR;
    bool Success = util::file_overwrite_content_sync(ResolutionFilePath,
                                                     JsonToDump.dump(2));
    if (!Success) {
      spdlog::warn("fail to write resolution result to file [{}]", DestPath);
    }
  } else { // merge resolution results
    std::ifstream ResolvedFS(ResolutionFilePath.string());
    nlohmann::json ResolvedJson = nlohmann::json::parse(ResolvedFS);
    server::FileResolutionResult FileResolved = ResolvedJson;
    std::vector<server::BlockResolutionResult> &Resolutions =
        FileResolved.resolutions;
#ifndef NDEBUG
    // this check is rather inefficient, and the situation is almost impossible
    // to happen, so we only perform this check at debug phase
    bool hasSame = utils::hasSameElements(
        Resolutions.begin(), Resolutions.end(), Results.begin(), Results.end(),
        [](server::BlockResolutionResult const &BR1,
           server::BlockResolutionResult const &BR2) {
          return BR1.index == BR2.index;
        });
    if (hasSame) {
      SPDLOG_ERROR(
          "Fatal logic error: there are overlaps between Resolutions and "
          "Results");
    }
#endif
    Resolutions.reserve(Resolutions.size() + Results.size());
    std::copy(Results.begin(), Results.end(), std::back_inserter(Resolutions));
    nlohmann::json JsonToDump = FileResolved;
    bool Success = util::file_overwrite_content_sync(ResolutionFilePath,
                                                     JsonToDump.dump(2));
    if (!Success) {
      spdlog::warn("fail to write resolution result to file [{}]", DestPath);
    }
  }
}

std::string pathToName(std::string_view path) {
  std::string sep = path.find('/') != std::string_view ::npos ? "/" : "\\";
  const std::string genericPath = fs::path(path).generic_string();

  std::vector<std::string_view> split =
      util::string_split(genericPath, sep, true);

  std::string replaced = util::string_join(split.begin(), split.end(), "@");
  std::replace(replaced.begin(), replaced.end(), ':', '#');
  return replaced;
}

std::string nameToPath(const std::string &name) {
  std::string sep = name.find('#') != std::string::npos ||
                            name.find("@@") != std::string::npos
                        ? "\\"
                        : "/";

  std::string replaced = name;
  std::replace(replaced.begin(), replaced.end(), '@',
               '/'); // Replace '@' with '/'
  std::replace(replaced.begin(), replaced.end(), '#',
               ':'); // Replace '~' with ':'

  std::vector<std::string> split;
  std::istringstream iss(replaced);
  std::string token;
  while (std::getline(iss, token, '/')) {
    split.push_back(token);
  }
  return util::string_join(split.begin(), split.end(), sep);
}

std::string_view extractCodeFromConflictRange(std::string_view Source,
                                              std::string_view StartMark,
                                              std::string_view EndMarker) {
  size_t StartPos = Source.find(StartMark);
  // NOLINT(google-readability-braces-around-statements)
  while (StartPos != std::string_view::npos && StartPos != Source.length() &&
         Source[StartPos++] != '\n')
    ;
  assert(StartPos != std::string_view::npos && StartPos != Source.length() &&
         "illegal conflict range, start marker line is in bad format");
  size_t EndPos = Source.find(EndMarker, StartPos);
  assert(EndPos != std::string_view::npos &&
         "illegal conflict range, no end marker");
  if (StartPos == std::string_view::npos || StartPos == Source.length() ||
      EndPos == std::string_view::npos) {
    return std::string_view();
  }
  return Source.substr(StartPos, EndPos - StartPos);
}

// 去除代码前后的空行和行内的注释
std::string trim(const std::string &code) {
  std::string str = code;
  // Remove leading blank lines
  while (str.length() > 0 && str[0] == '\n') {
    str.erase(0, 1);
  }
  // Remove trailing blank lines
  while (str.length() > 0 && str[str.length() - 1] == '\n') {
    str.erase(str.length() - 1, 1);
  }
  return str;
}

} // namespace sa
} // namespace mergebot
