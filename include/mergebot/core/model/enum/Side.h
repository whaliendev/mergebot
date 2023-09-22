//
// Created by whalien on 03/05/23.
//

#ifndef MB_SIDE_H
#define MB_SIDE_H
#include <magic_enum.hpp>
namespace mergebot {
namespace sa {
enum class Side : int {
  OURS,   // "ours"
  BASE,   // "base"
  THEIRS, // "theirs"
  OUT,    // "out"
};
}
} // namespace mergebot

namespace magic_enum {
namespace customize {
template <>
constexpr customize_t
enum_name<mergebot::sa::Side>(mergebot::sa::Side S) noexcept {
  using mergebot::sa::Side;
  switch (S) {
  case Side::OURS:
    return "ours";
  case Side::BASE:
    return "base";
  case Side::THEIRS:
    return "theirs";
  case Side::OUT:
    return "out";
  }
  return default_tag;
}
} // namespace customize
} // namespace magic_enum

#endif // MB_SIDE_H
