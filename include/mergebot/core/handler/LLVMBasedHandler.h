//
// Created by whalien on 13/03/23.
//

#ifndef MB_LLVMBASEDHANDLER_H
#define MB_LLVMBASEDHANDLER_H

#include "SAHandler.h"

namespace mergebot {
namespace sa {

class LLVMBasedHandler : public SAHandler {
public:
  LLVMBasedHandler(std::string Name = __FILE_NAME__) : SAHandler(Name) {}

private:
  void resolveConflictFiles(
      std::vector<ConflictFile> &ConflictFiles) const override {}
};
} // namespace sa
} // namespace mergebot

#endif // MB_LLVMBASEDHANDLER_H
