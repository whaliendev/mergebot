//
// Created by whalien on 21/02/23.
//
#include "mergebot/utility.h"

#include <llvm/Support/ErrorOr.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <spdlog/spdlog.h>
#include <sys/wait.h>

#include <array>
#include <cerrno>
#include <cstdio>
#include <magic_enum.hpp>
#include <memory>
#include <stdexcept>

#include "mergebot/controller/app_exception.h"
#include "mergebot/core/model/ConflictMark.h"
#include "mergebot/magic_enum_customization.h"
#include "mergebot/server/result_vo_utils.h"
#include "mergebot/utils/format.h"
#include "mergebot/utils/pathop.h"

namespace mergebot {
namespace util {
// git diff --name-only --diff-filter=U
/// Linux specific command execution encapsulation
/// \param cmd command to execute
/// \param timeout timout to execute the command, the unit is second.
/// \return `llvm::Error<std::string>`, an wrapper for std::error_code or the
/// output of `popen` returned stream, with the last new line removed
[[nodiscard]] llvm::ErrorOr<std::string> ExecCommand(std::string_view sv,
                                                     int timeout,
                                                     int exitCode) {
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
    if (res == -1) {  // exit accidentally
      return std::error_code(errno, std::generic_category());
    } else {                    // exit intentionally
      if (WIFEXITED(status)) {  // subprocess exit
        if (WEXITSTATUS(status) != 0 &&
            WEXITSTATUS(status) != exitCode) {  // exit accidentally
          spdlog::info("exit status: {}", WEXITSTATUS(status));
          return std::error_code(status, std::generic_category());
        }

        // exit successfully
        while (fgets(buffer.data(), buffer.size(), pipe.get())) {
          result += buffer.data();
        }
        if (result.length() && result[result.length() - 1]) result.pop_back();
        return result;
      } else if (WIFSIGNALED(status)) {  // interrupted by signal
        return std::make_error_code(std::errc::interrupted);
      } else {  // res equals to pid id
      }
    }
    // sleep for a second to prevent high cpu occupation
    usleep(1000);
  }
}
}  // namespace util

namespace sa {
namespace _details {
std::vector<ConflictBlock> constructConflictFile(
    std::unique_ptr<llvm::MemoryBuffer>& File) {
  int Index = 0;
  std::vector<ConflictBlock> ConflictBlocks;
  const char* Start = File->getBufferStart();
  const char* End = File->getBufferEnd();
  for (const char *Pos = Start, *LineStart = Start, *BlockStart = Start;
       Pos != End; LineStart = ++Pos) {
    Pos = std::find(Pos, End, '\n');
    std::string_view Line(LineStart, Pos - LineStart);
    if (Line.size() < 7) continue;
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
          Block.ConflictRange = std::string(BlockStart, Pos - BlockStart);
          ConflictBlocks.push_back(std::move(Block));
          break;
        }
        LineStart = ++Pos;
      }
    }
  }
  return ConflictBlocks;
}
}  // namespace _details

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

std::vector<ConflictFile> constructConflictFiles(
    std::vector<std::string>& ConflictFilePaths) {
  using namespace llvm;
  std::vector<ConflictFile> ConflictFiles;
  ConflictFiles.reserve(ConflictFilePaths.size());
  for (const auto& ConflictFilePath : ConflictFilePaths) {
    std::string AbsoluteFilePath = util::toabs(ConflictFilePath);
    ErrorOr<std::unique_ptr<MemoryBuffer>> FileOrErr =
        MemoryBuffer::getFile(AbsoluteFilePath);
    if (auto Err = FileOrErr.getError()) {
      spdlog::error(
          "failed to extract conflict blocks for source file [{}], err "
          "message: {}",
          AbsoluteFilePath, Err.message());
    }
    std::unique_ptr<MemoryBuffer> File = std::move(FileOrErr.get());
    std::vector<ConflictBlock> ConflictBlocks =
        _details::constructConflictFile(File);
    ConflictFiles.emplace_back(std::move(AbsoluteFilePath),
                               std::move(ConflictBlocks));
  }
  return ConflictFiles;
}
}  // namespace sa

namespace server {
void handleServerExecError(std::error_code err, std::string_view cmd) {
  if (err == std::errc::timed_out) {
    throw server::AppBaseException(
        "S1000", mergebot::util::format("timeout to executing {}", cmd));
  } else if (err == std::errc::interrupted) {
    throw server::AppBaseException(
        "S1000",
        mergebot::util::format("cmd [{}] accidentally interrupted", cmd));
  } else if (err == std::errc::io_error) {
    throw server::AppBaseException(
        "S1000", mergebot::util::format(
                     "open popen or waitpid failed for cmd [{}]", cmd));
  } else {
    throw server::AppBaseException(
        "S1000",
        mergebot::util::format("cmd [{}] accidentally exited with exit code {}",
                               cmd, err.value()));
  }
}

namespace ResultEnum {
// in C++, const vars at file scope is static linked unlike those in C.
const server::Result NO_ROUTE_MATCH(
    "C0001", "不存在匹配的路由记录，请检查请求路径或请求方法");
const server::Result BAD_REQUEST("C0002", "请求格式异常或参数错误");
}  // namespace ResultEnum
}  // namespace server

namespace server {
namespace ResultVOUtil {
void _regularizeRes(crow::response& res, crow::status code,
                    const std::string& body) {
  res.code = code;
  res.body = body;
  res.set_header("Content-Type", "application/json");
  res.end();
}

void return_success(crow::response& res,
                    const crow::json::wvalue& data = nullptr) {
  server::ResultVO rv("00000", "", data);
  _regularizeRes(res, crow::status::OK, rv.dump());
}

void return_error(crow::response& res, const server::Result& result) {
  server::ResultVO rv(result.code, result.errorMsg, nullptr);
  const auto code = result.code;
  auto status = crow::status::INTERNAL_SERVER_ERROR;
  if (code.length() && code[0] == 'C') {
    status = crow::status::BAD_REQUEST;
  } else if (code.length() && code[0] == 'S') {
    status = crow::status::INTERNAL_SERVER_ERROR;
  } else if (code.length() && code[0] == 'U') {
    status = crow::status::OK;
  }
  _regularizeRes(res, status, rv.dump());
}

void return_error(crow::response& res, const std::string& code,
                  const std::string& errorMsg) {
  SPDLOG_INFO("code: {}, errorMsg: {}", code, errorMsg);
  server::ResultVO rv(code, errorMsg, nullptr);
  auto status = crow::status::INTERNAL_SERVER_ERROR;
  if (code.length() && code[0] == 'C') {
    status = crow::status::BAD_REQUEST;
  } else if (code.length() && code[0] == 'S') {
    status = crow::status::INTERNAL_SERVER_ERROR;
  } else if (code.length() && code[0] == 'U') {
    status = crow::status::OK;
  }
  _regularizeRes(res, status, rv.dump());
}
}  // namespace ResultVOUtil
}  // namespace server
}  // namespace mergebot
