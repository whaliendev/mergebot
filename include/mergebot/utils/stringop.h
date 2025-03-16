//
// Created by whalien on 06/03/23.
//

#ifndef MB_STRINGOP_H
#define MB_STRINGOP_H

#include <re2/re2.h>

#include <algorithm>
#include <cassert>
#include <iterator>
#include <queue>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
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
  std::string result;
  result.reserve(code.size());

  bool inSingleLineComment = false;
  bool inMultiLineComment = false;
  bool inString = false;
  char stringDelimiter = 0;

  for (size_t i = 0; i < code.size(); ++i) {
    char c = code[i];
    char next = (i + 1 < code.size()) ? code[i + 1] : '\0';

    if (inString) {
      if (c == stringDelimiter && code[i - 1] != '\\') {
        inString = false;
      }
      result.push_back(c);
      continue;
    }

    if (c == '"' || c == '\'') {
      inString = true;
      stringDelimiter = c;
      result.push_back(c);
      continue;
    }

    if (inMultiLineComment) {
      if (c == '*' && next == '/') {
        inMultiLineComment = false;
        ++i;
      }
      continue;
    }

    if (inSingleLineComment) {
      if (c == '\n') {
        inSingleLineComment = false;
      }
      continue;
    }

    if (c == '/' && next == '/') {
      {
        inSingleLineComment = true;
        ++i;
        continue;
      }
    }

    if (c == '/' && next == '*') {
      {
        inMultiLineComment = true;
        ++i;
        continue;
      }
    }

    if (!std::isspace(static_cast<unsigned char>(c))) {
      {
        result.push_back(c);
      }
    }
  }

  return result;
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

static std::string doMacroExpansion(std::string_view code) {
  // define macro replacements
  static const std::vector<std::pair<std::string_view, std::string_view>>
      macroReplacements = {
          {"AT_LEAST_V_OR_202404",
           "API_LEVEL_AT_LEAST(__ANDROID_API_V__, 202404)"},
          {"AT_LEAST_U_OR_202304",
           "API_LEVEL_AT_LEAST(__ANDROID_API_U__, 202304)"},
          {"AT_LEAST_T_OR_202204",
           "API_LEVEL_AT_LEAST(__ANDROID_API_T__, 202204)"},
          {"AT_LEAST_S_OR_202104",
           "API_LEVEL_AT_LEAST(__ANDROID_API_S__, 202104)"},
          {"AT_LEAST_R_OR_202004",
           "API_LEVEL_AT_LEAST(__ANDROID_API_R__, 202004)"},
          {"AT_LEAST_Q_OR_201904",
           "API_LEVEL_AT_LEAST(__ANDROID_API_Q__, 201904)"},
          {"ANDROID_VERSION_V", "__ANDROID_API_V__"},
          {"ANDROID_VERSION_U", "__ANDROID_API_U__"},
          {"ANDROID_VERSION_T", "__ANDROID_API_T__"},
          {"ANDROID_VERSION_S", "__ANDROID_API_S__"},
          {"ANDROID_VERSION_R", "__ANDROID_API_R__"},
          {"ANDROID_VERSION_Q", "__ANDROID_API_Q__"},
          {"API_CHECK_V", "(__ANDROID_API__ >= __ANDROID_API_V__)"},
          {"API_CHECK_U", "(__ANDROID_API__ >= __ANDROID_API_U__)"},
          {"API_CHECK_T", "(__ANDROID_API__ >= __ANDROID_API_T__)"},
          {"API_CHECK_S", "(__ANDROID_API__ >= __ANDROID_API_S__)"},
          {"API_CHECK_R", "(__ANDROID_API__ >= __ANDROID_API_R__)"},
          {"API_CHECK_Q", "(__ANDROID_API__ >= __ANDROID_API_Q__)"}};

  std::string result{code};
  for (const auto& [macro, expansion] : macroReplacements) {
    size_t pos = 0;
    while ((pos = result.find(macro, pos)) != std::string::npos) {
      // make sure the macro is a whole word, in case of partial match
      bool isValidStart = (pos == 0 || !std::isalnum(result[pos - 1]));
      bool isValidEnd = (pos + macro.length() == result.length() ||
                         !std::isalnum(result[pos + macro.length()]));

      if (isValidStart && isValidEnd) {
        result.replace(pos, macro.length(), expansion);
        pos += expansion.length();
      } else {
        pos += 1;
      }
    }
  }
  return result;
}

class DependencyGraph {
 private:
  std::unordered_map<std::string, std::unordered_set<std::string>>
      adjacencyList;
  std::unordered_map<std::string, int> inDegree;
  std::unordered_set<std::string> visited;

  bool hasCycleDFS(const std::string& node,
                   std::unordered_set<std::string>& path) {
    visited.insert(node);
    path.insert(node);

    for (const auto& neighbor : adjacencyList[node]) {
      if (path.count(neighbor) > 0) {
        return true;  // Found cycle
      }
      if (visited.count(neighbor) == 0) {
        if (hasCycleDFS(neighbor, path)) {
          return true;
        }
      }
    }

    path.erase(node);
    return false;
  }

 public:
  bool addNode(const std::string& node) {
    if (adjacencyList.count(node) == 0) {
      adjacencyList[node] = std::unordered_set<std::string>();
      inDegree[node] = 0;
      return true;
    }
    return false;
  }

  bool addEdge(const std::string& from, const std::string& to) {
    if (adjacencyList.count(from) == 0 || adjacencyList.count(to) == 0) {
      return false;
    }
    adjacencyList[from].insert(to);
    inDegree[to]++;
    return true;
  }

  bool hasCycle() {
    visited.clear();
    std::unordered_set<std::string> path;

    for (const auto& node : adjacencyList) {
      if (visited.count(node.first) == 0) {
        if (hasCycleDFS(node.first, path)) {
          return true;
        }
      }
    }
    return false;
  }

  bool topologicalSort(std::vector<std::string>& result) {
    result.clear();
    result.reserve(adjacencyList.size());

    if (hasCycle()) {
      return false;
    }

    std::priority_queue<std::string, std::vector<std::string>, std::less<>> pq;
    auto tempInDegree = inDegree;

    // Find all nodes with in-degree 0
    for (const auto& node : adjacencyList) {
      if (tempInDegree[node.first] == 0) {
        pq.push(node.first);
      }
    }

    while (!pq.empty()) {
      auto current = pq.top();
      pq.pop();
      result.push_back(current);

      for (const auto& neighbor : adjacencyList[current]) {
        tempInDegree[neighbor]--;
        if (tempInDegree[neighbor] == 0) {
          pq.push(neighbor);
        }
      }
    }

    return result.size() == adjacencyList.size();
  }

  void clear() {
    adjacencyList.clear();
    inDegree.clear();
    visited.clear();
  }
};

}  // namespace util
}  // namespace mergebot
#endif  // MB_STRINGOP_H
