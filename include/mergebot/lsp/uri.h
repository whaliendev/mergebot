//
// Created by whalien on 11/06/23.
//

#ifndef MB_INCLUDE_MERGEBOT_LSP_URI_H
#define MB_INCLUDE_MERGEBOT_LSP_URI_H

#include <optional>
#include <string>
#include <string_view>
#include <tuple>

namespace mergebot {
namespace lsp {
class URI {
 public:
  URI(std::string_view scheme, std::string_view authority,
      std::string_view body);

  /// Returns decoded scheme e.g. "https"
  std::string_view scheme() const { return scheme_; }
  /// Returns decoded authority e.g. "reviews.lvm.org"
  std::string_view authority() const { return authority_; }
  /// Returns decoded body e.g. "/D41946"
  std::string_view body() const { return body_; }

  /// Returns a string URI with all components percent-encoded.
  std::string toString() const;

  /// Creates a URI for a file in the given scheme. \p Scheme must be
  /// registered. The URI is percent-encoded.
  // currently, we only support file scheme
  //  static std::optional<URI> create(std::string_view absolutePath,
  //                                   std::string_view scheme);

  // Similar to above except this picks a registered scheme that works. If none
  // works, this falls back to "file" scheme.
  //  static URI create(std::string_view absolutePath);

  /// This creates a file:// URI for \p AbsolutePath. The path must be absolute.
  static URI createFile(std::string_view absolutePath);

  /// Parse a URI string "<scheme>:[//<authority>/]<path>". Percent-encoded
  /// characters in the URI will be decoded.
  static std::optional<URI> parse(std::string_view uri);

  /// Resolves the absolute path of \p U. If there is no matching scheme, or the
  /// URI is invalid in the scheme, this returns an error.
  ///
  /// \p HintPath A related path, such as the current file or working directory,
  /// which can help disambiguate when the same file exists in many workspaces.
  //  static std::optional<std::string> resolve(const URI &uri,
  //                                            std::string_view hintPath = "");

  /// Same as above, in addition it parses the \p FileURI using URI::parse.
  //  static std::optional<std::string> resolve(std::string_view fileURI,
  //                                            std::string_view hintPath = "");

  /// Resolves \p AbsPath into a canonical path of its URI, by converting
  /// \p AbsPath to URI and resolving the URI to get th canonical path.
  /// This ensures that paths with the same URI are resolved into consistent
  /// file path.
  //  static std::optional<std::string> resolvePath(std::string_view absPath,
  //                                                std::string_view hintPath =
  //                                                "");

  /// Gets the preferred spelling of this file for #include, if there is one,
  /// e.g. <system_header.h>, "path/to/x.h".
  ///
  /// This allows URI schemas to provide their customized include paths.
  ///
  /// Returns an empty string if normal include-shortening based on the absolute
  /// path should be used.
  /// Fails if the URI is not valid in the schema.
  //  static std::optional<std::string> includeSpelling(const URI &uri);

  friend bool operator==(const URI &lhs, const URI &rhs) {
    return std::tie(lhs.scheme_, lhs.authority_, lhs.body_) ==
           std::tie(rhs.scheme_, rhs.authority_, rhs.body_);
  }

  friend bool operator<(const URI &lhs, const URI &rhs) {
    return std::tie(lhs.scheme_, lhs.authority_, lhs.body_) <
           std::tie(rhs.scheme_, rhs.authority_, rhs.body_);
  }

 private:
  URI() = default;
  static void percentEncode(std::string_view content, std::string &out);
  static std::string percentDecode(std::string_view content);
  static bool shouldEscape(unsigned char ch);

  std::string scheme_;
  std::string authority_;
  std::string body_;
};

// URI in "file" scheme for a file.
struct URIForFile {
  URIForFile() = default;
  URIForFile(const std::string &file) : file(file) {}
  URIForFile(std::string &&file) : file(std::move(file)) {}

  /// Canonicalizes \p AbsPath via URI.
  ///
  /// File paths in URIForFile can come from index or local AST. Path from
  /// index goes through URI transformation, and the final path is resolved by
  /// URI scheme and could potentially be different from the original path.
  /// Hence, we do the same transformation for all paths.
  ///
  /// Files can be referred to by several paths (e.g. in the presence of links).
  /// Which one we prefer may depend on where we're coming from. \p TUPath is a
  /// hint, and should usually be the main entrypoint file we're processing.
  //  static URIForFile canonicalize(std::string_view absPath,
  //                                 std::string_view TUPath);

  static std::optional<URIForFile> fromURI(std::string_view uri,
                                           std::string_view hintPath = "");

  explicit operator bool() const { return !file.empty(); }
  std::string uri() const { return URI::createFile(file).toString(); }
  std::string path() const { return std::string(URI::createFile(file).body()); }

  friend bool operator==(const URIForFile &LHS, const URIForFile &RHS) {
    return LHS.file == RHS.file;
  }

  friend bool operator!=(const URIForFile &LHS, const URIForFile &RHS) {
    return !(LHS == RHS);
  }

  friend bool operator<(const URIForFile &LHS, const URIForFile &RHS) {
    return LHS.file < RHS.file;
  }

  std::string file;
};
}  // namespace lsp
}  // namespace mergebot
#endif  // MB_INCLUDE_MERGEBOT_LSP_URI_H
