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
  std::string project;
  std::string path;
  std::string cacheDir;
  std::vector<MergeScenario> mss;

  Project() = default;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Project, project, path,
                                                cacheDir, mss);
} // namespace sa
} // namespace mergebot
#endif // MB_PROJECT_H
