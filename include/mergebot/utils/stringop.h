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
std::vector<std::string_view> string_split(std::string_view str,
                                           std::string_view delims);

template <typename InputIt>
std::string string_join(InputIt begin, InputIt end,
                        const std::string_view separator) {
  static_assert(
      std::is_same<typename std::iterator_traits<InputIt>::iterator_category,
                   std::random_access_iterator_tag>::value,
      "string_join only supports random access iterators");

  std::ostringstream oss;
  if (begin != end) {
    oss << *begin++;
  }
  std::ostream_iterator<typename std::iterator_traits<InputIt>::value_type> it(
      oss, separator.data());
  std::copy(begin, end, it);
  return oss.str();
}

/// join Container of string by separator
/// \tparam Container the string container which supports random access
/// iterator. Specifically, `std::begin()` and `std::end()` \param cont the
/// string container \param separator separator to join string \return a concat
/// string joined by separator
template <typename Container>
typename std::enable_if<
    std::is_same<typename std::decay<
                     decltype(*std::begin(std::declval<Container>()))>::type,
                 typename std::decay<decltype(*std::end(
                     std::declval<Container>()))>::type>::value,
    std::string>::type
string_join(const Container& cont, const std::string_view separator) {
  return string_join(std::begin(cont), std::end(cont), separator);
}

static std::string removeSpaces(std::string_view sv) {
  std::string result;
  std::remove_copy_if(sv.begin(), sv.end(), std::back_inserter(result),
                      [](char c) { return std::isspace(c); });
  return result;
}
}  // namespace util
}  // namespace mergebot
#endif  // MB_STRINGOP_H
