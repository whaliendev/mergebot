//
// Created by whalien on 13/03/23.
//

#ifndef MB_HANDLERCHAIN_H
#define MB_HANDLERCHAIN_H

#include "mergebot/core/handler/SAHandler.h"
#include "sa_utility.h"
#include <iomanip>
#include <ios>
#include <spdlog/spdlog.h>
#include <sstream>

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
    ConflictBlockCount = countConflictBlocks();
    spdlog::info("there are {} conflict blocks in this merge scenario",
                 ConflictBlockCount);
    Handlers_[0]->handle(ConflictFiles_);
    int AfterResolveCount = countConflictBlocks();
    spdlog::info("there are still {} conflict blocks in this merge scenario",
                 AfterResolveCount);
    //    double Ratio = ((ConflictBlockCount - AfterResolveCount) /
    //                    static_cast<double>(ConflictBlockCount)) *
    //                   100;
    //    std::stringstream SS;
    //    SS << std::fixed << std::setprecision(2) << Ratio << " %";
    //    spdlog::info("resolve ratio at this merge scenario is: {}", SS.str());
    writeResolveRatio(ConflictBlockCount,
                      ConflictBlockCount - AfterResolveCount);
  }

private:
  void writeResolveRatio(int cbCnt, int resolvedCnt);
  int ConflictBlockCount = 0;
  void chain();
  unsigned countConflictBlocks() const;
  std::vector<ConflictFile> ConflictFiles_;
  std::vector<std::unique_ptr<SAHandler>> Handlers_;
};
} // namespace sa
} // namespace mergebot

#endif // MB_HANDLERCHAIN_H
