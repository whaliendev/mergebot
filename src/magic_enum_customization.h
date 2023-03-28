//
// Created by whalien on 28/03/23.
//

#ifndef MB_MAGIC_ENUM_CUSTOMIZATION_H
#define MB_MAGIC_ENUM_CUSTOMIZATION_H

#include <magic_enum.hpp>

#include "utility.h"

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
}  // namespace customize
}  // namespace magic_enum

#endif  // MB_MAGIC_ENUM_CUSTOMIZATION_H
