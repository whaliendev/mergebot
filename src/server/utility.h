//
// Created by whalien on 16/02/23.
//

#ifndef MB_UTILITY_H
#define MB_UTILITY_H
#include <string>

#include "result_vo_utils.h"

namespace mergebot {
namespace util {
// git diff --name-only --diff-filter=U
std::string ExecCommand(const char* cmd);
}  // namespace util

namespace server {
inline bool noerr(crow::json::wvalue& rv) {
  return !(rv.t() == crow::json::type::Object &&
           std::find(rv.keys().begin(), rv.keys().end(), "error") != rv.keys().end() &&
           rv["error"].dump() == "true");
}

namespace ResultEnum {
// TODO(hwa): every static var is independent, not global var
// return code:
//  U: user side error. U0xxx, static constructed, U1, dynamic constructed.
//  C: client error.
//  S: server side error
const static server::Result NO_ROUTE_MATCH("U0001", "不存在匹配的路由记录");
const static server::Result BAD_REQUEST("C0001", "请求格式异常或参数错误");
}  // namespace ResultEnum

namespace ResultVOUtil {
void return_success(crow::response& res, const crow::json::wvalue& data = nullptr);

void return_error(crow::response& res, const server::Result& result);

void return_error(crow::response& res, const std::string& code, const std::string& errorMsg);
}  // namespace ResultVOUtil
}  // namespace server
}  // namespace mergebot

#endif  // MB_UTILITY_H
