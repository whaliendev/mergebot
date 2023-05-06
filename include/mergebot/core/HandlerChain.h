//
// Created by whalien on 13/03/23.
//

#ifndef MB_HANDLERCHAIN_H
#define MB_HANDLERCHAIN_H

#include "mergebot/core/handler/SAHandler.h"
#include "sa_utility.h"
#include <spdlog/spdlog.h>

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

  void handle() {
    spdlog::info("there are {} conflict blocks in this merge scenario",
                 countConflictBlocks());
    Handlers_[0]->handle(ConflictFiles_);
    spdlog::info("there are still {} conflict blocks in this merge scenario",
                 countConflictBlocks());
  }

private:
  void chain();
  unsigned countConflictBlocks() const;
  std::vector<ConflictFile> ConflictFiles_;
  std::vector<std::unique_ptr<SAHandler>> Handlers_;
};
} // namespace sa
} // namespace mergebot

#endif // MB_HANDLERCHAIN_H
