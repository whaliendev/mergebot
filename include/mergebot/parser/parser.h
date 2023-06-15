//
// Created by whalien on 07/05/23.
//

#ifndef MB_PARSER_H
#define MB_PARSER_H

#include <tree_sitter/api.h>

#include <vector>

#include "mergebot/parser/range.h"
#include "tree.h"
#include "tree_sitter/api.h"

extern "C" {
namespace mergebot {
namespace ts {
class Parser {
 public:
  /// create a new parser, with language filed null
  Parser();
  /// create a new parser and set analysis language
  explicit Parser(TSLanguage const*);
  /// delete parser
  ~Parser();

  /// parse a source buffer and return a concrete syntax tree.
  ///
  /// If you are parsing this document for the first time, pass `nullptr` for
  /// the `old_tree` parameter. Otherwise, if you have already parsed an earlier
  /// version of this document and the document has since been edited, pass the
  /// previous syntax tree so that the unchanged parts of it can be reused.
  /// This will save time and memory. For this to work correctly, you must have
  /// already edited the old syntax tree using the TODO(hwa): `ts_tree_edit`
  /// function in a way that exactly matches the source code changes.
  ///
  /// **the source buffer is assumed to be encoded in UTF-8**
  ///
  /// This function returns a syntax tree on success, and `nullptr` on failure.
  /// There are two possible reasons for failure(cancellation flag is not
  /// implemented):
  /// 1. The parser does not have a language assigned. Check for this using the
  ///    `setLanguage` function.
  /// 2. Parsing was cancelled due to a timeout that was set by an earlier call
  /// to the `setTimeoutMicros` function. You can resume parsing from where the
  /// parser left out by calling `parse` again with the same arguments. Or you
  /// can start parsing from scratch by first calling `resetParsing`.
  /// 3. Parsing was cancelled due to a cancellation flag set by an earlier call
  /// to `cancelParsing`. You can resume parsing from where the parser left out
  /// by calling `parse` again with the same arguments. Refer to `cancelParsing`
  std::shared_ptr<Tree> parse(std::string const& source,
                              Tree* old_tree = nullptr,
                              bool keep_text = true) const;

  /**
   * Set the language that the parser should use for parsing.
   *
   * the caller should make sure `TSLanguage` version conform to the ABI
   * constraint. Namely, `TREE_SITTER_LANGUAGE_VERSION` and
   * `TREE_SITTER_MIN_COMPATIBLE_LANGUAGE_VERSION` constraint will be checked
   * inside this mutator
   */
  void setLanguage(TSLanguage const*);

  const TSLanguage* language() const;

  /**
   * Set the ranges of text that the parser should include when parsing.
   *
   * By default, the parser will always include entire documents. This function
   * allows you to parse only a *portion* of a document but still return a
   * syntax tree whose ranges match up with the document as a whole. You can
   * also pass multiple disjoint ranges.
   *
   * Note that Ranges in ranges vector should not overlap, or this operation
   * will fail. On success, it will return true. When `ranges.size()` is 0, the
   * entire document will be parsed.
   */
  bool setIncludedRanges(std::vector<Range> const& ranges) const;
  std::vector<Range> includedRanges() const;

  /**
   * Set the maximum duration in microseconds that parsing should be allowed to
   * take before halting.
   *
   * If parsing takes longer than this, it will halt early, returning nullptr.
   * See `parse` for more information.
   */
  void setTimeoutMicros(uint64_t timeout) const;
  uint64_t timeoutMicros() const;

  /**
   * Instruct the parser to start the next parse from the beginning.
   *
   * If the parser previously failed because of a timeout or a cancellation,
   * then by default, it will resume where it left off on the next call to
   * `parse` or other parsing functions. If you don't want to resume,
   * and instead intend to use this parser to parse some other document, you
   * must call `resetParsing` first.
   */
  void resetParsing() const;

  void cancelParsing();
  bool cancelled() const;

  std::string nameOfSymbol(TSSymbol) const;

  /**
   * Set the file descriptor to which the parser should write debugging graphs
   * during parsing. The graphs are formatted in the DOT language. You may want
   * to pipe these graphs directly to a `dot(1)` process in order to generate
   * SVG output. You can turn off this logging by passing a negative number.
   */
  void exportToDotGraphs(int file) const;

 private:
  TSParser* parser;
  size_t* cancel_flag;
};
}  // namespace ts
}  // namespace mergebot
};

#endif  // MB_PARSER_H
