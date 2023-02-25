//
// Created by whalien on 21/02/23.
//
#include <array>
#include <cstdio>
#include <memory>
#include <stdexcept>

#include "mergebot/utils/format.h"
#include "result_vo_utils.h"

namespace mergebot {
namespace util {
// git diff --name-only --diff-filter=U
std::string ExecCommand(const char* cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
    throw std::runtime_error(format("execute {} failed", cmd));
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}
}  // namespace util
namespace server {
namespace ResultVOUtil {
void __regularizeRes(crow::response& res, crow::status code,
                     const std::string& body) {
  res.code = code;
  res.body = body;
  res.set_header("Content-Type", "application/json");
  res.end();
}

void return_success(crow::response& res,
                    const crow::json::wvalue& data = nullptr) {
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

void return_error(crow::response& res, const std::string& code,
                  const std::string& errorMsg) {
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
