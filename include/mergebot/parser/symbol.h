//
// Created by whalien on 07/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_PARSER_SYMBOL_H
#define MB_INCLUDE_MERGEBOT_PARSER_SYMBOL_H

#include <tree_sitter/api.h>

#include <ostream>

extern "C" {
namespace mergebot {
namespace ts {

class Symbol {
 public:
  constexpr Symbol(TSSymbol symbol, const char* name)
      : symbol(symbol), name(name) {}

  constexpr operator TSSymbol() const { return symbol; }

  TSSymbol symbol;
  const char* name;
};

}  // namespace ts
}  // namespace mergebot
};

#endif  // MB_INCLUDE_MERGEBOT_PARSER_SYMBOL_H
