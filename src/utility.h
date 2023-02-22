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

namespace ResultEnum {
// TODO(hwa): every static var is independent, not global var
const static server::ResultEnum NO_ROUTE_MATCH("U0001",
                                               "不存在匹配的路由记录");
const static server::ResultEnum NOT_A_GIT_REPO("U0002",
                                               "所传入项目路径非git仓库");
const static server::ResultEnum NO_CONFLICTS_FOUND("U0003",
                                                   "当前项目路径下无冲突文件");
const static server::ResultEnum BAD_REQUEST("C0001", "请求格式异常或参数错误");
}  // namespace ResultEnum

namespace ResultVOUtil {
server::ResultVO success(const crow::json::wvalue& data = nullptr);

server::ResultVO error(const server::ResultEnum& result);

server::ResultVO error(const std::string& code, const std::string& errorMsg);
}  // namespace ResultVOUtil
}  // namespace mergebot

#endif  // MB_UTILITY_H
