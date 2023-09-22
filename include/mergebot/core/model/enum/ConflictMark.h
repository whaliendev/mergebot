//
// Created by whalien on 31/03/23.
//

#ifndef MB_CONFLICTMARK_H
#define MB_CONFLICTMARK_H

#include <magic_enum.hpp>

namespace mergebot {
namespace sa {
enum class ConflictMark : int {
  OURS,   // <<<<<<<
  BASE,   // |||||||
  THEIRS, // =======
  END     // >>>>>>>
};
}
} // namespace mergebot

namespace magic_enum {
namespace customize {
template <>
constexpr customize_t enum_name<mergebot::sa::ConflictMark>(
    mergebot::sa::ConflictMark Mark) noexcept {
  using mergebot::sa::ConflictMark;
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
} // namespace customize
} // namespace magic_enum

#endif // MB_CONFLICTMARK_H
