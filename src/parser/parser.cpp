//
// Created by whalien on 07/05/23.
//
#include "mergebot/parser/parser.h"

#include <tree_sitter/api.h>

#include <cassert>
#include <vector>

#include "mergebot/parser/range.h"

namespace mergebot {
namespace ts {
Parser::Parser() : parser(ts_parser_new()), cancel_flag(nullptr) {}

Parser::Parser(const TSLanguage *language)
    : parser(ts_parser_new()), cancel_flag(nullptr) {
  ts_parser_set_language(parser, language);
}
Parser::~Parser() { ts_parser_delete(parser); }

std::shared_ptr<Tree> Parser::parse(const std::string &source, Tree *old_tree,
                                    bool keep_text) const {
  ts_parser_set_cancellation_flag(parser, cancel_flag);
  TSTree *new_tree = ts_parser_parse_string(
      parser, old_tree != nullptr ? old_tree->tree : nullptr, source.c_str(),
      source.length());
  return Tree::create(new_tree, source, keep_text);
}

void Parser::setLanguage(const TSLanguage *language) {
  assert(language && "language pointer cannot be null");
  unsigned version = ts_language_version(language);
  assert(
      version <= TREE_SITTER_LANGUAGE_VERSION &&
      TREE_SITTER_MIN_COMPATIBLE_LANGUAGE_VERSION <= version &&
      "language version incompatible with tree sitter, which will broke ABI");
  ts_parser_set_language(parser, language);
}

const TSLanguage *Parser::language() const {
  return ts_parser_language(parser);
}

std::vector<Range> Parser::includedRanges() const {
  uint32_t length = 0;
  const Range *parser_arr = ts_parser_included_ranges(parser, &length);
  return std::vector<Range>(parser_arr, parser_arr + length);
}

bool Parser::setIncludedRanges(const std::vector<Range> &ranges) const {
  Range range_arr[ranges.size()];
  return ts_parser_set_included_ranges(parser, range_arr, ranges.size());
}

void Parser::cancelParsing() { *cancel_flag = 1; }

bool Parser::cancelled() const {
  return *ts_parser_cancellation_flag(parser) != 0;
}
void Parser::setTimeoutMicros(uint64_t timeout) const {
  ts_parser_set_timeout_micros(parser, timeout);
}

uint64_t Parser::timeoutMicros() const {
  return ts_parser_timeout_micros(parser);
}

void Parser::resetParsing() const { ts_parser_reset(parser); }

void Parser::exportToDotGraphs(int file) const {
  ts_parser_print_dot_graphs(parser, file);
}
std::string Parser::nameOfSymbol(TSSymbol symbol) const {
  return ts_language_symbol_name(language(), symbol);
}
}  // namespace ts
}  // namespace mergebot