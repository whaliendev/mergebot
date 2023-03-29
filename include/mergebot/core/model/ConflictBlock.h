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
  /// Conflict Range content, starts with "<<<<<<<", ends with ">>>>>>>"
  std::string ConflictRange;

  operator std::string() {
    return fmt::format("ConflictBlock(Index = {}, ConflictRange = {},)", Index,
                       ConflictRange);
  }

  friend bool operator==(ConflictBlock const &Lhs, ConflictBlock const &Rhs) {
    return Lhs.Index == Rhs.Index && Lhs.ConflictRange == Rhs.ConflictRange;
  }
};
} // namespace sa
} // namespace mergebot

#endif // MB_CONFLICTBLOCK_H
