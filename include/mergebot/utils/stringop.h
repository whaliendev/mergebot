//
// Created by whalien on 06/03/23.
//

#ifndef MB_STRINGOP_H
#define MB_STRINGOP_H

#include <string_view>
#include <vector>

namespace mergebot {
namespace util {
std::vector<std::string_view> string_split(std::string_view str,
                                           std::string_view delims = " ") {
  std::vector<std::string_view> output;
  // output.reserve(str.size() / 2);

  for (auto first = str.data(), second = str.data(), last = first + str.size();
       second != last && first != last; first = second + 1) {
    second =
        std::find_first_of(first, last, std::cbegin(delims), std::cend(delims));

    if (first != second) output.emplace_back(first, second - first);
  }

  return output;
}
}  // namespace util
}  // namespace mergebot
#endif  // MB_STRINGOP_H
