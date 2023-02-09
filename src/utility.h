//
// Created by whalien on 16/02/23.
//

#ifndef MB_UTILITY_H
#define MB_UTILITY_H

#include <array>
#include <cstdio>
#include <memory>
#include <stdexcept>

#include "base/String.h"

namespace mergebot {
namespace util {
// git diff --name-only --diff-filter=U
String ExecCommand(const char* cmd) {
  std::array<char, 128> buffer;
  String result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}
}  // namespace util
}  // namespace mergebot

#endif  // MB_UTILITY_H
