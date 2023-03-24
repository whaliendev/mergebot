//
// Created by whalien on 16/02/23.
//

#ifndef MB_UTILITY_H
#define MB_UTILITY_H
#include <llvm/Support/ErrorOr.h>
#include <spdlog/spdlog.h>

#include <string>
#include <string_view>

#include "core/model/ConflictFile.h"
#include "server/result_vo_utils.h"

namespace mergebot {
namespace util {
// git diff --name-only --diff-filter=U
llvm::ErrorOr<std::string> ExecCommand(std::string_view sv, int timeout = 10,
                                       int exitCode = 0);
}  // namespace util

namespace sa {
void handleSAExecError(std::error_code err, std::string_view cmd);

std::vector<ConflictFile> extractConflictBlocks(
    std::vector<std::string>& ConflictFiles);
}  // namespace sa

namespace server {
void handleServerExecError(std::error_code err, std::string_view cmd);
inline bool err(crow::json::wvalue& rv) {
  return rv.t() == crow::json::type::Object &&
         std::find(rv.keys().begin(), rv.keys().end(), "error") !=
             rv.keys().end() &&
         rv["error"].dump().size();
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

void return_error(crow::response& res, const std::string& code,
                  const std::string& errorMsg);
}  // namespace ResultVOUtil
}  // namespace server
}  // namespace mergebot

#endif  // MB_UTILITY_H
