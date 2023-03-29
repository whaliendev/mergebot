//
// Created by whalien on 28/02/23.
//

#ifndef MB_PATHOP_H
#define MB_PATHOP_H
#include <spdlog/spdlog.h>

#include <cassert>

#include "../filesystem.h"
namespace mergebot {
namespace util {
static std::string toabs(std::string const& pathStr) {
  assert(pathStr.size());
  fs::path path = pathStr;
  std::string ret;
  if (path.is_relative()) {
    if (pathStr[0] == '~') {
      // Unix specific
      fs::path homePath = fs::path(getenv("HOME"));
      ret = fs::path(homePath.string() + path.string().substr(1)).string();
    } else {
      ret = fs::canonical(path).string();
    }
  } else {
    ret = path.string();
  }
  return ret;
}
}  // namespace util
}  // namespace mergebot

#endif  // MB_PATHOP_H
