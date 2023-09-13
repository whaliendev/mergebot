//
// Created by whalien on 13/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_UTILS_FMT_ESCAPE_H
#define MB_INCLUDE_MERGEBOT_UTILS_FMT_ESCAPE_H
#include <fmt/format.h>

namespace mergebot::util {
//! A wrapper holding a view that should be escaped when output.
template <typename StringView = std::string_view>
struct ViewToEscape {
  // Data is just a string view and the character for a quote and for escaping.
  StringView sv;
  typename StringView::value_type quote = '"';
  typename StringView::value_type esc = '\\';

  //! Copy to an output iterator: Quotes and escapes need escaping:
  template <typename OutCharIter>
  OutCharIter copy(OutCharIter out) const {
    *out++ = this->quote;  // Start with a quote.
    auto needsEscaping = [this](auto c) {
      return c == this->quote || c == this->esc;
    };
    auto it = this->sv.begin();
    for (const auto& c : this->sv) {
      if (needsEscaping(c)) {
        *out++ = this->esc;
      }
      *out++ = c;
    }
    *out++ = this->quote;  // Final quote.
    return out;
  }
};

constexpr ViewToEscape<std::string_view> escaped(std::string_view sv,
                                                 char q = '"', char e = '\\') {
  return ViewToEscape<std::string_view>{sv, q, e};
}
}  // namespace mergebot::util

template <typename StringView>
struct fmt::formatter<mergebot::util::ViewToEscape<StringView>> {
  using V = mergebot::util::ViewToEscape<StringView>;

  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(mergebot::util::ViewToEscape<StringView> const& s,
              FormatContext& ctx) {
    return s.copy(ctx.out());
  }
};

#endif  // MB_INCLUDE_MERGEBOT_UTILS_FMT_ESCAPE_H
