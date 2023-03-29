//
// Created by whalien on 29/03/23.
//
#include "mergebot/utils/stringop.h"

namespace mergebot {
namespace util {
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
}  // namespace util
}  // namespace mergebot
