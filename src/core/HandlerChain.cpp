//
// Created by whalien on 15/03/23.
//

#include "mergebot/core/HandlerChain.h"
#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/globals.h"
#include "mergebot/utils/pathop.h"
#include <fstream>
#include <memory>
#include <numeric>
#include <vector>

namespace mergebot {
namespace sa {
void HandlerChain::chain() {
  assert(!this->Handlers_.empty() && this->Handlers_[0].get() &&
         "the handlers in Handler Chain cannot be empty");
  for (size_t cur = 0; cur + 1 < this->Handlers_.size(); ++cur) {
    this->Handlers_[cur]->setNext(this->Handlers_[cur + 1].get());
  }
}

unsigned HandlerChain::countConflictBlocks() const {
  return std::accumulate(ConflictFiles_.begin(), ConflictFiles_.end(), 0,
                         [&](int Cnt, const ConflictFile &Cur) {
                           return Cnt + Cur.ConflictBlocks.size();
                         });
}

void HandlerChain::writeResolveRatio(int cbCnt, int resolvedCnt) {
  fs::path ratioPath = fs::path(util::toabs(MBDIR)) / "ratio.csv";
  FILE *file = fopen(ratioPath.c_str(), "a");
  if (file == nullptr) {
    spdlog::error("unable to write to file\n");
    return;
  }

  int number1 = cbCnt;
  int number2 = resolvedCnt;
  fprintf(file, "%d, %d\n", number1, number2);

  spdlog::info("write to ratio file successfully");
  fclose(file);
}
} // namespace sa
} // namespace mergebot
