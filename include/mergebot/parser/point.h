//
// Created by whalien on 07/05/23.
//

#ifndef MB_POINT_H
#define MB_POINT_H

#include <fmt/core.h>
#include <tree_sitter/api.h>

#include <iostream>
namespace mergebot {
namespace ts {

using Point = TSPoint;

}  // namespace ts
}  // namespace mergebot
extern std::ostream& operator<<(std::ostream&, mergebot::ts::Point const&);

namespace fmt {

template <>
struct formatter<mergebot::ts::Point> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    // Check if reached the end of the range:
    //    if (it != end && *it != '}')
    //      throw fmt::format_error("invalid format");
    // Return an iterator past the end of the parsed range:
    return it;
  }

  template <typename FormatContext>
  auto format(const mergebot::ts::Point& p, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(), "Point({}, {})", p.row, p.column);
  }
};

}  // namespace fmt

#endif  // MB_POINT_H
