//
// Created by whalien on 08/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_EDGEKIND_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_EDGEKIND_H

#include <array>
#include <cstdint>
#include <magic_enum.hpp>

namespace mergebot {
namespace sa {
enum class EdgeKind : uint8_t {
  ILLEGAL,
  CONTAIN,
  INCLUDE,
  REFERENCE,
  DEFINE,
  USE
};
} // namespace sa
} // namespace mergebot

namespace magic_enum {
namespace customize {
using mergebot::sa::EdgeKind;
template <> constexpr customize_t enum_name<EdgeKind>(EdgeKind Kind) noexcept {
  switch (Kind) {
  case EdgeKind::ILLEGAL:
    return "ILLEGAL";
  case EdgeKind::CONTAIN:
    return "CONTAIN";
  case EdgeKind::INCLUDE:
    return "INCLUDE";
  case EdgeKind::REFERENCE:
    return "REF";
  case EdgeKind::DEFINE:
    return "DEFINE";
  case EdgeKind::USE:
    return "USE";
  }
  return default_tag;
}
} // namespace customize
} // namespace magic_enum

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_EDGEKIND_H
