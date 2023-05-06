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
namespace util {
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
} // namespace util

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
  if (!fs::exists(ResolutionFilePath) ||
      (fs::exists(ResolutionFilePath) &&
       !nlohmann::json::accept(ResolutionFS))) {
    server::FileResolutionResult FR{.filepath = std::string(FileName),
                                    .resolutions = Results};
    nlohmann::json JsonToDump = FR;
    util::file_overwrite_content(ResolutionFilePath, JsonToDump.dump(2));
  } else { // merge resolution results
    std::ifstream ResolvedFS(ResolutionFilePath.string());
    nlohmann::json ResolvedJson = nlohmann::json::parse(ResolvedFS);
    server::FileResolutionResult FileResolved = ResolvedJson;
    std::vector<server::BlockResolutionResult> &Resolutions =
        FileResolved.resolutions;
#ifndef NDEBUG
    // this check is rather inefficient and the situation is almost impossible
    // to happen, so we only perform this check at debug phase
    bool hasSame = util::hasSameElements(
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
    util::file_overwrite_content(ResolutionFilePath, JsonToDump.dump(2));
  }
}

std::string pathToName(std::string_view path) {
  // FIX(hwa): use a less common path separator
  return util::string_join(util::string_split(path, "/"), "-");
}

std::string nameToPath(std::string_view name) {
  std::vector<std::string_view> Segs = util::string_split(name, "-");
  return std::accumulate(Segs.begin(), Segs.end(), fs::path("/"),
                         [](fs::path const &P, std::string_view seg) {
                           return P / fs::path(seg);
                         })
      .string();
}

} // namespace sa
} // namespace mergebot