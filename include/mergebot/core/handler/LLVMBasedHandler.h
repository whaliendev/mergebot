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
  explicit LLVMBasedHandler(ProjectMeta Meta, std::string Name = __FILE_NAME__)
      : SAHandler(Meta, Name) {}

private:
  void resolveConflictFiles(std::vector<ConflictFile> &ConflictFiles) override {
  }
};
} // namespace sa
} // namespace mergebot

#endif // MB_LLVMBASEDHANDLER_H
