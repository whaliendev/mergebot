//
// Created by whalien on 04/03/23.
//

#include "ResolutionManager.h"
#include <spdlog/spdlog.h>

namespace mergebot {
namespace sa {
void ResolutionManager::doResolution() {
  // remember to set running sign
  // clang-format off
  spdlog::info(R"(begin resolving conflicts...
    project: {},
    project path: {},
    merge scenario: {},
    conflict files: [
        {}
    ]
  )", Project_, ProjectPath_, MS_,
  fmt::join(ConflictFiles_.get(), ConflictFiles_.get() + FileNum_, ",\n\t\t"));
  // clang-format on
}
} // namespace sa
} // namespace mergebot
