//
// Created by whalien on 13/03/23.
//

#ifndef MB_CONFLICTBLOCK_H
#define MB_CONFLICTBLOCK_H
#include <fmt/format.h>

#include <string>
namespace mergebot {
namespace sa {
struct ConflictLines {
  /// start line of a conflict block
  int Start = 0;
  /// end line of a conflict block
  int Offset = 0;

  std::string string() {
    return fmt::format(R"(ConflictLies(Start = {}, Offset = {}))", Start,
                       Offset);
  }
};

struct ConflictBlock {
  int Index = 0;
  /// our commit
  ConflictLines Ours;
  /// their commit
  ConflictLines Theirs;
  /// base commit
  ConflictLines Base;

  std::string string() {
    // clang-format off
    return fmt::format(R"(ConflictBlock(
        Index = {},
        Ours = {},
        Base = {},
        Theirs = {},
    ))", Index, Ours.string(), Base.string(), Theirs.string());
    // clang-format on
  }
};
} // namespace sa
} // namespace mergebot

#endif // MB_CONFLICTBLOCK_H
