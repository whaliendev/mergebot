//
// Created by whalien on 30/04/23.
//

#include "mergebot/core/semantic/GraphBuilder.h"

#include "mergebot/filesystem.h"
#include "mergebot/lsp/client.h"
#include <magic_enum.hpp>

namespace mergebot {
namespace sa {
bool GraphBuilder::build() {
  for (std::string const &Path : SourceList) {
    processTranslationUnit(Path);
  }
  return true;
}
void GraphBuilder::processTranslationUnit(std::string_view Path) {}
} // namespace sa
} // namespace mergebot