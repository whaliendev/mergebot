//
// Created by whalien on 13/03/23.
//

#ifndef MB_RESOLUTIONRESULTVO_H
#define MB_RESOLUTIONRESULTVO_H

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace mergebot {
namespace sa {
struct BlockResolutionResult {
  int index;
  std::string msg;
  std::string code;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BlockResolutionResult, index,
                                                msg, code);

struct ResolutionResultVO {
  std::string project;
  std::string file;
  std::vector<BlockResolutionResult> results;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ResolutionResultVO, project,
                                                file, results);
} // namespace sa
} // namespace mergebot
#endif // MB_RESOLUTIONRESULTVO_H
