//
// Created by whalien on 21/02/23.
//
#include "utility.h"

#include <spdlog/spdlog.h>

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
  // TODO(hwa): change return type to ErrorOr<>
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
    throw std::runtime_error(format("execute {} failed", cmd));
  }
  while (fread(buffer.data(), 1, buffer.size(), pipe.get())) {
    result += buffer.data();
  }
  if (result.length() && result[result.length() - 1]) result.pop_back();
  return result;
}
}  // namespace util

namespace server {
namespace ResultEnum {
// in C++, const vars at file scope is static linked unlike that in C.
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
