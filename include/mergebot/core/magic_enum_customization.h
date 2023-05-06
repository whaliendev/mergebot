//
// Created by whalien on 28/03/23.
//

#ifndef MB_MAGIC_ENUM_CUSTOMIZATION_H
#define MB_MAGIC_ENUM_CUSTOMIZATION_H

#include "magic_enum.hpp"

#include "model/ConflictMark.h"
#include "model/Side.h"

namespace magic_enum {
namespace customize {
using mergebot::sa::ConflictMark;
template <>
constexpr customize_t enum_name<ConflictMark>(ConflictMark Mark) noexcept {
  switch (Mark) {
  case ConflictMark::OURS:
    return "<<<<<<<";
  case ConflictMark::BASE:
    return "|||||||";
  case ConflictMark::THEIRS:
    return "=======";
  case ConflictMark::END:
    return ">>>>>>>";
  }
  return default_tag;
}

using mergebot::sa::Side;
template <> constexpr customize_t enum_name<Side>(Side S) noexcept {
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

#endif // MB_MAGIC_ENUM_CUSTOMIZATION_H
