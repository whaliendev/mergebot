//
// Created by whalien on 21/02/23.
//
#include <array>
#include <cstdio>
#include <memory>
#include <stdexcept>

#include "result_vo_utils.h"
#include "mergebot/utils/format.h"

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

namespace ResultVOUtil {
server::ResultVO success(const crow::json::wvalue& data = nullptr) {
  server::ResultVO rv("00000", "", data);
  return rv;
}

server::ResultVO error(const server::ResultEnum& result) {
  server::ResultVO rv(result.code, result.errorMsg, nullptr);
  return rv;
}

server::ResultVO error(const std::string& code, const std::string& errorMsg) {
  server::ResultVO rv(code, errorMsg, nullptr);
  return rv;
}
}  // namespace ResultVOUtil
}  // namespace mergebot
