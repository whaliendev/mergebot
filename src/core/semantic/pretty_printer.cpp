//
// Created by whalien on 30/04/23.
//

#include "mergebot/core/semantic/pretty_printer.h"
#include "mergebot/core/model/node/TranslationUnitNode.h"
#include "mergebot/utils/fileio.h"
namespace mergebot::sa {
namespace details {}

std::string PrettyPrintTU(const std::shared_ptr<SemanticNode> &TUNode,
                          const std::string &DestDir) {
  assert(llvm::isa<TranslationUnitNode>(TUNode.get()));
  TranslationUnitNode *TURawPtr = llvm::cast<TranslationUnitNode>(TUNode.get());
  std::string DestFile = (fs::path(DestDir) / TURawPtr->DisplayName).string();
  /// TODO(fyc): print logic
  std::string PrintSource = "TODO";
  util::file_overwrite_content(DestFile, PrintSource);
  return DestFile;
}
} // namespace mergebot::sa
