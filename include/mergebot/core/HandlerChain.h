//
// Created by whalien on 13/03/23.
//

#ifndef MB_HANDLERCHAIN_H
#define MB_HANDLERCHAIN_H

#include "mergebot/core/handler/SAHandler.h"
#include "mergebot/utility.h"

namespace mergebot {
namespace sa {
class HandlerChain {
public:
  HandlerChain(std::vector<std::unique_ptr<SAHandler>> &&Handlers,
               std::vector<std::string> ConflictFilePaths)
      : Handlers_(std::move(Handlers)) {
    // construct handler chain
    chain();
    ConflictFiles_ = constructConflictFiles(ConflictFilePaths);
  }

  void handle() { Handlers_[0]->handle(ConflictFiles_); }

private:
  void chain();
  std::vector<ConflictFile> ConflictFiles_;
  std::vector<std::unique_ptr<SAHandler>> Handlers_;
};
} // namespace sa
} // namespace mergebot

#endif // MB_HANDLERCHAIN_H
