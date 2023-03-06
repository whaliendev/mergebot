//
// Created by whalien on 21/02/23.
//
#include "utility.h"

#include <llvm/Support/ErrorOr.h>
#include <spdlog/spdlog.h>
#include <sys/wait.h>

#include <array>
#include <cstdio>
#include <memory>
#include <stdexcept>

#include "../controller/app_exception.h"
#include "mergebot/utils/format.h"
#include "result_vo_utils.h"

namespace mergebot {
namespace util {
// git diff --name-only --diff-filter=U
/// Linux specific command execution encapsulation
/// \param cmd command to execute
/// \param timeout timout to execute the command, unit is second.
/// \return `llvm::Error<std::string>`, an wrapper for std::error_code or the output of `popen`
/// returned stream, with the last new line removed
llvm::ErrorOr<std::string> ExecCommand(std::string_view sv, int timeout) {
  std::string result;

  std::array<char, 128> buffer;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(sv.data(), "r"), pclose);
  if (!pipe) {
    return std::make_error_code(std::errc::io_error);
  }

  int pid = 0, status = 0;
  auto start_time = std::chrono::steady_clock::now();
  while (true) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
    if (elapsed_time >= timeout) {
      pipe.reset();
      return std::make_error_code(std::errc::timed_out);
    }

    int res = waitpid(0, &status, WNOHANG);
    if (res == -1) {  // exit accidentally
      return std::make_error_code(std::errc::io_error);
    } else if (res > 0) {              // exit normally
      if (WIFEXITED(status)) {         // subprocess exit
        if (WEXITSTATUS(status) != 0)  // exit accidentally
          return std::error_code(status, std::generic_category());

        // exit successfully
        while (fgets(buffer.data(), buffer.size(), pipe.get())) {
          result += buffer.data();
        }
        if (result.length() && result[result.length() - 1]) result.pop_back();
        return result;
      } else if (WIFEXITED(status)) {  // interrupted
        return std::make_error_code(std::errc::interrupted);
      }
    }
    // sleep for a second to prevent high cpu occupation
    usleep(1000);
  }
}

void handleServerExecError(std::error_code err, std::string_view cmd) {
  if (err == std::errc::timed_out) {
    throw server::AppBaseException("S1000", mergebot::util::format("timeout to executing {}", cmd));
  } else if (err == std::errc::interrupted) {
    throw server::AppBaseException(
        "S1000", mergebot::util::format("cmd [{}] accidentally interrupted", cmd));
  } else if (err == std::errc::io_error) {
    throw server::AppBaseException(
        "S1000", mergebot::util::format("open popen or waitpid failed for cmd [{}]", cmd));
  } else {
    throw server::AppBaseException(
        "S1000",
        mergebot::util::format("cmd [{}] accidentally exited with exit code {}", cmd, err.value()));
  }
}
}  // namespace util

namespace server {
namespace ResultEnum {
// in C++, const vars at file scope is static linked unlike those in C.
const server::Result NO_ROUTE_MATCH("C0001", "不存在匹配的路由记录，请检查请求路径或请求方法");
const server::Result BAD_REQUEST("C0002", "请求格式异常或参数错误");
}  // namespace ResultEnum
}  // namespace server

namespace server {
namespace ResultVOUtil {
void __regularizeRes(crow::response& res, crow::status code, const std::string& body) {
  res.code = code;
  res.body = body;
  res.set_header("Content-Type", "application/json");
  res.end();
}

void return_success(crow::response& res, const crow::json::wvalue& data = nullptr) {
  server::ResultVO rv("00000", "", data);
  __regularizeRes(res, crow::status::OK, rv.dump());
}

void return_error(crow::response& res, const server::Result& result) {
  server::ResultVO rv(result.code, result.errorMsg, nullptr);
  const auto code = result.code;
  auto status = crow::status::INTERNAL_SERVER_ERROR;
  if (code.length() && code[0] == 'C') {
    status = crow::status::BAD_REQUEST;
  } else if (code.length() && code[0] == 'S') {
    status = crow::status::INTERNAL_SERVER_ERROR;
  }
  __regularizeRes(res, status, rv.dump());
}

void return_error(crow::response& res, const std::string& code, const std::string& errorMsg) {
  server::ResultVO rv(code, errorMsg, nullptr);
  auto status = crow::status::INTERNAL_SERVER_ERROR;
  if (code.length() && code[0] == 'C') {
    status = crow::status::BAD_REQUEST;
  } else if (errorMsg.length() && errorMsg[0] == 'S') {
    status = crow::status::INTERNAL_SERVER_ERROR;
  }
  __regularizeRes(res, status, rv.dump());
}
}  // namespace ResultVOUtil
}  // namespace server
}  // namespace mergebot
