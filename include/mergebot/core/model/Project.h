//
// Created by whalien on 28/02/23.
//

#ifndef MB_PROJECT_H
#define MB_PROJECT_H

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "MergeScenario.h"

namespace mergebot {
namespace sa {
struct Project {
  // Note that these public members need to violate LLVM's coding standard as it
  // will be used as json key when performing serialization and deserialization
  std::string project;
  std::string path;
  std::string cacheDir;
  std::vector<MergeScenario> mss;

  explicit operator std::string() const {
    // clang-format off
    return fmt::format(R"(Project(
        project = {},
        path = {},
        cacheDir = {},
        mss = [{}]
    ))", project, path, cacheDir, fmt::join(mss, ", "));
    // clang-format on
  }
  Project() = default;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Project, project, path,
                                                cacheDir, mss);
} // namespace sa
} // namespace mergebot
#endif // MB_PROJECT_H
