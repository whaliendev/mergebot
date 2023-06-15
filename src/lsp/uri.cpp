//
// Created by whalien on 11/06/23.
//

#include "mergebot/lsp/uri.h"

#include <cassert>
#include <string_view>

#include "mergebot/utils/stringop.h"

namespace mergebot {
namespace lsp {
namespace _details {
enum class Style {
  native,
  posix,
  windows_slash,
  windows_backslash,
  windows = windows_backslash,  // deprecated
};

/// Check if \p S uses POSIX path rules.
constexpr bool is_style_posix(Style s) {
  if (s == Style::posix) return true;
  if (s != Style::native) return false;
#if defined(_WIN32)
  return false;
#else
  return true;
#endif
}

constexpr bool is_style_windows(Style s) { return !is_style_posix(s); }

bool is_separator(char value, Style style = Style::native) {
  if (value == '/') return true;
  if (is_style_windows(style)) return value == '\\';
  return false;
}

inline const char* separators(Style style) {
  if (is_style_windows(style)) return "\\/";
  return "/";
}

std::string_view find_first_component(std::string_view path, Style style) {
  // Look for this first component in the following order.
  // * empty (in this case we return an empty string)
  // * either C: or {//,\\}net.
  // * {/,\}
  // * {file,directory}name

  if (path.empty()) return path;

  if (is_style_windows(style)) {
    // C:
    if (path.size() >= 2 && std::isalpha(static_cast<unsigned char>(path[0])) &&
        path[1] == ':')
      return path.substr(0, 2);
  }

  // //net
  if ((path.size() > 2) && is_separator(path[0], style) && path[0] == path[1] &&
      !is_separator(path[2], style)) {
    // Find the next directory separator.
    size_t end = path.find_first_of(separators(style), 2);
    return path.substr(0, end);
  }

  // {/,\}
  if (is_separator(path[0], style)) return path.substr(0, 1);

  // * {file,directory}name
  size_t end = path.find_first_of(separators(style));
  return path.substr(0, end);
}

std::string_view root_name(std::string_view path, Style style = Style::native) {
  std::string_view first_component = find_first_component(path, style);
  if (path.size()) {
    // Windows UNC path \\server\share\{path}
    bool has_net = first_component.size() > 2 && is_separator(path[0], style) &&
                   path[1] == path[0];
    bool has_drive =
        is_style_windows(style) && util::ends_with(first_component, ":");

    if (has_net || has_drive) {
      return first_component;
    }
  }
  return {};
}

bool isNetworkPath(std::string_view Path) {
  return Path.size() > 2 && Path[0] == Path[1] && is_separator(Path[0]);
}

bool isWindowsPath(std::string_view path) {
  return path.size() > 1 && isalpha(path[0]) && path[1] == ':';
}

std::string convert_to_slash(std::string_view path,
                             Style style = Style::native) {
  if (is_style_posix(style)) return std::string(path);

  std::string s = std::string(path);
  std::replace(s.begin(), s.end(), '\\', '/');
  return s;
}

bool isValidScheme(std::string_view scheme) {
  if (scheme.empty()) return false;
  if (!isalpha(scheme[0])) return false;
  scheme = scheme.substr(1);
  return std::all_of(scheme.begin(), scheme.end(), [&](const auto& item) {
    return isalnum(item) || item == '+' | item == '.' || item == '-';
  });
}
}  // namespace _details

URI::URI(std::string_view scheme, std::string_view authority,
         std::string_view body)
    : scheme_(scheme), authority_(authority), body_(body) {
  assert(!scheme.empty());
  assert((authority_.empty() || util::starts_with(body_, "/")) &&
         "URI body must start with '/' when authority is present");
}
std::string URI::toString() const {
  std::string out;
  percentEncode(scheme_, out);
  out.push_back(':');
  if (authority_.empty() && body_.empty()) return out;
  // If authority is empty, we only print body if it starts with "/"; otherwise,
  // the URI is invalid.
  if (!authority_.empty() || util::starts_with(body_, "/")) {
    out.append("//");
    percentEncode(authority_, out);
  }
  percentEncode(body_, out);
  return out;
}
void URI::percentEncode(std::string_view content, std::string& out) {
  for (unsigned char C : content)
    if (shouldEscape(C)) {
      out.push_back('%');
      out.push_back(util::hexdigit(C / 16));
      out.push_back(util::hexdigit(C % 16));
    } else {
      out.push_back(C);
    }
}
bool URI::shouldEscape(unsigned char ch) {
  // Unreserved characters.
  if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
      (ch >= '0' && ch <= '9'))
    return false;
  switch (ch) {
    case '-':
    case '_':
    case '.':
    case '~':
    case '/':  // '/' is only reserved when parsing.
    // ':' is only reserved for relative URI paths, which clangd doesn't
    // produce.
    case ':':
      return false;
  }
  return true;
}
URI URI::createFile(std::string_view absolutePath) {
  std::string body;
  std::string_view authority;
  std::string_view root = _details::root_name(absolutePath);
  if (_details::isNetworkPath(root)) {
    // Windows UNC paths e.g. \\server\share => file://server/share
    assert(root.size() > 2 &&
           "size of root of network path must be greater than 2");
    authority = root.substr(2);
    absolutePath = absolutePath.substr(root.size());
  } else if (_details::isWindowsPath(root)) {
    // Windows paths e.g. X:\path => file:///X:/path
    body = "/";
  }

  body += _details::convert_to_slash(absolutePath);
  return URI("file", authority, body);
}
std::string URI::percentDecode(std::string_view content) {
  std::string out;
  for (auto i = content.begin(), e = content.end(); i != e; ++i) {
    if (*i != '%') {
      out += *i;
      continue;
    }
    if (*i == '%' && i + 2 < content.end() && util::isHexDigit(*(i + 1)) &&
        util::isHexDigit(*(i + 2))) {
      out.push_back(util::hexFromNibbles(*(i + 1), *(i + 2)));
      i += 2;
    } else {
      out.push_back(*i);
    }
  }
  return out;
}

std::optional<URI> URI::parse(std::string_view origUri) {
  URI out;
  std::string_view uri = origUri;

  // https://username:password@example.com:8080/path/to/resource
  auto pos = uri.find(':');
  if (pos == std::string_view::npos) return std::nullopt;
  auto schemeStr = uri.substr(0, pos);
  out.scheme_ = percentDecode(schemeStr);
  if (!_details::isValidScheme(out.scheme_)) return std::nullopt;
  uri = uri.substr(pos + 1);

  bool authorDoubleSlash = util::starts_with(uri, "//");
  if (authorDoubleSlash) uri = uri.substr(2);
  if (authorDoubleSlash) {
    pos = uri.find('/');
    out.authority_ = percentDecode(uri.substr(0, pos));
    uri = uri.substr(pos);
  }
  out.body_ = percentDecode(uri);
  return out;
}

std::optional<URIForFile> URIForFile::fromURI(std::string_view uri,
                                              std::string_view hintPath) {
  auto uriOpt = URI::parse(uri);
  assert(uriOpt && "illegal uri");
  // use file path to construct a URIForFile
  return URIForFile(std::string(uriOpt->body()));
}
}  // namespace lsp
}  // namespace mergebot
