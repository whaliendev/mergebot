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
// return code:
//  U: user side error. U0xxx, static constructed, U1, dynamic constructed.
//  C: client error.
//  S: server side error
extern const server::Result NO_ROUTE_MATCH;
extern const server::Result BAD_REQUEST;
}  // namespace ResultEnum

namespace ResultVOUtil {
void return_success(crow::response& res, const crow::json::wvalue& data);

void return_error(crow::response& res, const server::Result& result);

void return_error(crow::response& res, const std::string& code, const std::string& errorMsg);
}  // namespace ResultVOUtil
}  // namespace server
}  // namespace mergebot

#endif  // MB_UTILITY_H
