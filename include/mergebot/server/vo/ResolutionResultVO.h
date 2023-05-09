//
// Created by whalien on 13/03/23.
//

#ifndef MB_RESOLUTIONRESULTVO_H
#define MB_RESOLUTIONRESULTVO_H

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "mergebot/utils/String.h"

namespace mergebot {
namespace server {
struct BlockResolutionResult {
  /// resolution block index
  int index = -1;
  /// description message to help manually resolve
  std::string desc = "";
  /// resolution code to apply
  std::string code = "";

  explicit operator std::string() const {
    return fmt::format(
        "BlockResolutionResult(index = {}, desc = {}, code = {})", index, desc,
        code);
  }

  friend bool operator==(BlockResolutionResult const &Lhs,
                         BlockResolutionResult const &Rhs) {
    return Lhs.index == Rhs.index && Lhs.desc == Rhs.desc &&
           Lhs.code == Rhs.code;
  }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BlockResolutionResult, index,
                                                desc, code);
struct FileResolutionResult {
  std::string filepath;
  std::vector<BlockResolutionResult> resolutions;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FileResolutionResult, filepath,
                                                resolutions);
}  // namespace server
}  // namespace mergebot
#endif  // MB_RESOLUTIONRESULTVO_H
