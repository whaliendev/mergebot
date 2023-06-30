//
// Created by whalien on 06/05/23.
//

#ifndef MB_SERVER_UTILITY_H
#define MB_SERVER_UTILITY_H

#include <crow/http_response.h>
#include <crow/json.h>

#include <system_error>

#include "mergebot/core/model/MergeScenario.h"
#include "mergebot/server/result_vo_utils.h"
#include "mergebot/server/vo/ResolutionResultVO.h"

namespace mergebot {
namespace server {

namespace utils {
bool checkMSMetadata(const std::string& project, const std::string& path,
                     const sa::MergeScenario& ms);
void checkConflictFile(const std::string& project, const std::string& path,
                       const sa::MergeScenario& ms, const std::string& file);
void checkPath(std::string const& pathStr);
void checkGitRepo(std::string const& path);
bool containKeys(const crow::json::rvalue& bodyJson,
                 const std::vector<std::string>& keys);
std::string calcProjChecksum(std::string const& project,
                             std::string const& path);
void validateAndCompleteCommitHash(sa::MergeScenario& ms,
                                   const std::string& projectPath);
}  // namespace utils

void handleServerExecError(std::error_code err, std::string_view cmd);
inline bool err(crow::json::wvalue& rv) {
//  return rv.t() == crow::json::type::Object &&
//         std::find(rv.keys().begin(), rv.keys().end(), "error") !=
//             rv.keys().end() &&
//         rv["error"].dump() != "null";
    return rv.t() == crow::json::type::Object &&
      rv.count("error") && rv["error"].dump() != "null";
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
                  const std::string& msg);
}  // namespace ResultVOUtil
}  // namespace server
}  // namespace mergebot

#endif  // MB_SERVER_UTILITY_H
