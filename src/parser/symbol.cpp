//
// Created by whalien on 07/05/23.
//

#include "mergebot/parser/symbol.h"

namespace mergebot {
namespace ts {
std::string Symbol::nameOfSymbol(TSLanguage const* language, TSSymbol sym) {
  return ts_language_symbol_name(language, sym);
}
}  // namespace ts
}  // namespace mergebot