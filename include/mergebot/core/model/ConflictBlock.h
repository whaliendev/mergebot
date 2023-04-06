//
// Created by whalien on 13/03/23.
//

#ifndef MB_CONFLICTBLOCK_H
#define MB_CONFLICTBLOCK_H
#include <fmt/format.h>

#include <spdlog/spdlog.h>
#include <string>

namespace mergebot {
namespace sa {
struct ConflictLines {
  /// start line of a conflict block
  int Start = 0;
  /// offset of a conflict block
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

  /// whether this block is resolved
  bool Resolved = false;

  operator std::string() {
    return fmt::format(
        "ConflictBlock(Index = {}, ConflictRange = {}, Resolved = {})", Index,
        ConflictRange, Resolved);
  }

  friend bool operator==(ConflictBlock const &Lhs, ConflictBlock const &Rhs) {
    return Lhs.Index == Rhs.Index && Lhs.ConflictRange == Rhs.ConflictRange &&
           Lhs.Resolved == Rhs.Resolved;
  }
};
} // namespace sa
} // namespace mergebot

// define the formatter struct template for ConflictBlock
template <> struct fmt::formatter<mergebot::sa::ConflictBlock> {
  // define the format-spec for ConflictBlock
  char presentation = 's';

  // implement the parse method
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    // parse the format-spec
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && (*it == 's' || *it == 'r')) {
      presentation = *it++;
    }

    // check if parsed correctly, throw exception otherwise
    if (it != end) {
      throw fmt::format_error("invalid format");
    }

    // return the iterator after parsing
    return it;
  }

  // implement the format method
  template <typename FormatContext>
  auto format(mergebot::sa::ConflictBlock const &cb, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    // format the ConflictBlock data based on the presentation
    if (presentation == 'r') {
      return format_to(ctx.out(),
                       "{{Index = {}, ConflictRange = {}, Resolved = {}}}",
                       cb.Index, cb.ConflictRange, cb.Resolved);
    } else {
      return format_to(
          ctx.out(),
          "ConflictBlock(Index = {}, ConflictRange = {}, Resolved = {})",
          cb.Index, cb.ConflictRange, cb.Resolved);
    }
  }
};

#endif // MB_CONFLICTBLOCK_H
