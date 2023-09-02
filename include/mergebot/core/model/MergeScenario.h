//
// Created by whalien on 09/02/23.
//

#ifndef MB_MERGESCENARIO_H
#define MB_MERGESCENARIO_H

#include <fmt/format.h>

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

  MergeScenario(std::string ours, std::string theirs, std::string base)
      : ours(ours), theirs(theirs), base(base) {
    name = fmt::format("{}-{}", ours.substr(0, 6), theirs.substr(0, 6));
  }

  //  explicit operator std::string() const {
  //    // clang-format off
  //    return fmt::format(
  // R"(MergeScenario(
  //      name = {},
  //      ours = {},
  //      theirs = {},
  //      base = {}
  //    ))", name, ours, theirs, base);
  //    // clang-format on
  //  }
  std::string toString() const {
    return fmt::format(
        "MergeScenario(name = {}, ours = {}, theirs = {}, base = {})", name,
        ours, theirs, base);
  }

  std::string toPrettyString() const {
    // clang-format off
    return fmt::format(
R"(MergeScenario(
      name = {},
      ours = {},
      theirs = {},
      base = {}
    ))", name, ours, theirs, base);
    // clang-format on
  }
  MergeScenario() = default;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MergeScenario, name, ours,
                                                theirs, base);

} // namespace sa
} // namespace mergebot

template <> struct fmt::formatter<mergebot::sa::MergeScenario> {
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    // Check if reached the end of the range:
    //    if (it != end && *it != '}')
    //      throw fmt::format_error("invalid format");
    // Return an iterator past the end of the parsed range:
    return it;
  }

  // Formats the point p using the parsed format specification (presentation)
  // stored in this formatter.
  template <typename FormatContext>
  auto format(const mergebot::sa::MergeScenario &ms, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.
    return fmt::format_to(ctx.out(), "{}", ms.toString());
  }
};

#endif // MB_MERGESCENARIO_H
