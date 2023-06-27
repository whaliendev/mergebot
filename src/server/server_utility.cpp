//
// Created by whalien on 06/05/23.
//

#include <spdlog/spdlog.h>

#include <system_error>

#include "mergebot/controller/app_exception.h"
#include "mergebot/server/result_vo_utils.h"
#include "mergebot/utils/format.h"

namespace mergebot {
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
  server::ResultVO rv(result.code, result.msg, nullptr);
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
                  const std::string& msg) {
  SPDLOG_INFO("code: {}, msg: {}", code, msg);
  server::ResultVO rv(code, msg, nullptr);
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