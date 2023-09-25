//
// Created by whalien on 06/03/23.
//

#ifndef MB_STRINGOP_H
#define MB_STRINGOP_H

#include <re2/re2.h>

#include <algorithm>
#include <cassert>
#include <iterator>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <vector>

namespace mergebot {
namespace util {
/// Interpret the given character \p C as a hexadecimal digit and return its
/// value.
///
/// If \p C is not a valid hex digit, -1U is returned.
inline unsigned hexDigitValue(char C) {
  /* clang-format off */
  static const int16_t LUT[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,  // '0'..'9'
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 'A'..'F'
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 'a'..'f'
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  };
  /* clang-format on */
  return LUT[static_cast<unsigned char>(C)];
}

/// hexdigit - Return the hexadecimal character for the
/// given number \p X (which should be less than 16).
inline char hexdigit(unsigned X, bool LowerCase = false) {
  assert(X < 16);
  static const char LUT[] = "0123456789ABCDEF";
  const uint8_t Offset = LowerCase ? 32 : 0;
  return LUT[X] | Offset;
}

/// Checks if character \p C is a hexadecimal numeric character.
inline bool isHexDigit(char C) { return hexDigitValue(C) != ~0U; }

/// Store the binary representation of the two provided values, \p MSB and
/// \p LSB, that make up the nibbles of a hexadecimal digit. If \p MSB or \p LSB
/// do not correspond to proper nibbles of a hexadecimal digit, this method
/// returns false. Otherwise, returns true.
inline bool tryGetHexFromNibbles(char MSB, char LSB, uint8_t& Hex) {
  unsigned U1 = hexDigitValue(MSB);
  unsigned U2 = hexDigitValue(LSB);
  if (U1 == ~0U || U2 == ~0U) return false;

  Hex = static_cast<uint8_t>((U1 << 4) | U2);
  return true;
}

/// Return the binary representation of the two provided values, \p MSB and
/// \p LSB, that make up the nibbles of a hexadecimal digit.
inline uint8_t hexFromNibbles(char MSB, char LSB) {
  uint8_t Hex = 0;
  bool GotHex = tryGetHexFromNibbles(MSB, LSB, Hex);
  (void)GotHex;
  assert(GotHex && "MSB and/or LSB do not correspond to hex digits");
  return Hex;
}

template <
    typename Container = std::vector<std::string_view>,
    typename = std::enable_if_t<
        std::is_same_v<typename Container::value_type, std::string_view> ||
        std::is_same_v<typename Container::value_type, std::string>>>
Container string_split(std::string_view str, std::string_view delims = " ",
                       bool with_empty = false) {
  Container output;

  for (auto first = str.data(), second = str.data(), last = first + str.size();
       second != last && first != last; first = second + 1) {
    second =
        std::find_first_of(first, last, std::cbegin(delims), std::cend(delims));

    if (first != second || with_empty) {
      output.emplace_back(first, second - first);
    }
  }

  return output;
}

inline std::string_view string_trim(std::string_view str,
                                    std::string_view whitespace = " \t\n\r") {
  auto start = str.find_first_not_of(whitespace);
  if (start == std::string_view::npos) {
    return {};  // Empty or all space
  }
  auto end = str.find_last_not_of(whitespace);
  return str.substr(start, end - start + 1);
}

/// split string_view str by any of the elements in [ std::cbegin(delims),
/// std::cend(delims) )
/// \param str string_view to split
/// \param delims delimiters used to split
/// \return a vector of string_view with split string_view
/// segments filled
// std::vector<std::string_view> string_split(std::string_view str,
//                                            std::string_view delims,
//                                            bool with_empty = false);

template <typename InputIt>
std::string string_join(InputIt begin, InputIt end,
                        const std::string_view separator) {
  static_assert(
      std::is_same<typename std::iterator_traits<InputIt>::iterator_category,
                   std::random_access_iterator_tag>::value,
      "string_join only supports random access iterators");

  std::ostringstream oss;
  if (begin == end) return "";

  std::ostream_iterator<typename std::iterator_traits<InputIt>::value_type> it(
      oss, separator.data());
  std::copy(begin, end, it);
  std::string concatenated = oss.str();
  size_t pos = concatenated.find_last_of(separator);
  if (pos != std::string::npos) {
    concatenated = concatenated.substr(0, pos);
  }
  return concatenated;
}

/// join Container of string by separator
/// \tparam Container the string container which supports random access
/// iterator. Specifically, `std::begin()` and `std::end()`
/// \param cont the string containers
/// \param separator separator to join string
/// \return a concat string joined by separator
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

static std::string removeCommentsAndSpaces(std::string&& code) {
  // remove line comments, m means make ^, $ match line begin/end in addition to
  // text begin/end
  re2::RE2::GlobalReplace(&code, "(?m)//.*", "");
  // remove block comments, s means make dot match newline
  re2::RE2::GlobalReplace(&code, "(?s)/\\*.*?\\*/", "");
  // remove spaces, tabs, newlines, and other whitespace characters
  re2::RE2::GlobalReplace(&code, "\\s+", "");
  return code;
}

static bool starts_with(std::string_view str, std::string_view prefix) {
  if (prefix.length() > str.length()) {
    return false;
  }
  return str.compare(0, prefix.length(), prefix) == 0;
}

static bool ends_with(const std::string_view& str, const std::string& suffix) {
  if (suffix.length() > str.length()) {
    return false;
  }
  return str.compare(str.length() - suffix.length(), suffix.length(), suffix) ==
         0;
}

/// \brief check if two strings are equal, ignoring spaces
/// \param old_str the old string
/// \param new_str the new string
/// \return true if two strings are equal, ignoring spaces
inline bool diff_only_in_spaces(std::string_view old_str,
                                std::string_view new_str) {
  auto left1 = old_str.begin();
  auto left2 = new_str.begin();
  auto right1 = old_str.end();
  auto right2 = new_str.end();

  while (left1 <= right1 && left2 <= right2) {
    while (left1 <= right1 && std::isspace(static_cast<unsigned char>(*left1)))
      ++left1;
    while (left2 <= right2 && std::isspace(static_cast<unsigned char>(*left2)))
      ++left2;
    while (left1 <= right1 && std::isspace(static_cast<unsigned char>(*right1)))
      --right1;
    while (left2 <= right2 && std::isspace(static_cast<unsigned char>(*right2)))
      --right2;

    if ((left1 <= right1 && left2 > right2) ||
        (left1 > right1 && left2 <= right2)) {
      return false;  // One string has non-whitespace characters while the other
                     // doesn't
    }

    if (*left1 != *left2) {
      return false;  // Mismatch found
    }

    if (left1 <= right1 && left2 <= right2) {
      ++left1;
      ++left2;
    }
  }

  return true;
}

}  // namespace util
}  // namespace mergebot
#endif  // MB_STRINGOP_H
