//
// Created by whalien on 28/02/23.
//

#ifndef MB_GLOBALS_H
#define MB_GLOBALS_H

#include <mutex>
#include <string>
#include <vector>

namespace mergebot {
extern const std::string MBDIR;
extern std::vector<std::mutex> MANIFEST_LOCKS;
}  // namespace mergebot

#endif  // MB_GLOBALS_H
