//
// Created by whalien on 09/02/23.
//

#ifndef MB_MERGESCENARIO_H
#define MB_MERGESCENARIO_H

#include <nlohmann/json.hpp>
#include <string>

namespace mergebot {
namespace sa {
struct MergeScenario {
  // Note that these public members need to violate LLVM's coding standard as it
  // will be used as json key when performing serialization and deserialization
  std::string name;
  std::string ours;
  std::string theirs;
  std::string base;

  MergeScenario() = default;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MergeScenario, name, ours,
                                                theirs, base);

} // namespace sa
} // namespace mergebot

#endif // MB_MERGESCENARIO_H
