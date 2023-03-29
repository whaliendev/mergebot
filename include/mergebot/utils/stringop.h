//
// Created by whalien on 06/03/23.
//

#ifndef MB_STRINGOP_H
#define MB_STRINGOP_H

#include <algorithm>
#include <iterator>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <vector>

namespace mergebot {
namespace util {
/// split string_view str by any of the elements in [ std::cbegin(delims),
/// std::cend(delims) ) \param str string_view to split \param delims delimiters
/// used to split \return a vector of string_view with split string_view
/// segments filled
static std::vector<std::string_view> string_split(
    std::string_view str, std::string_view delims = " ") {
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

template <typename Container>
typename std::enable_if<
    std::is_same<typename std::decay<
                     decltype(*std::begin(std::declval<Container>()))>::type,
                 typename std::decay<decltype(*std::end(
                     std::declval<Container>()))>::type>::value,
    std::string>::type
string_join(const Container& cont, const std::string_view separator);

template <typename InputIt>
std::string string_join(InputIt begin, InputIt end,
                        const std::string_view separator);
}  // namespace util
}  // namespace mergebot
#endif  // MB_STRINGOP_H
