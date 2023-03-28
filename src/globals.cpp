//
// Created by whalien on 28/02/23.
//

#include "mergebot/globals.h"

namespace mergebot {
const std::string MBDIR = "~/.mergebot";
std::vector<std::mutex> MANIFEST_LOCKS(16);
}  // namespace mergebot
