//
// Created by whalien on 15/03/23.
//

#include "mergebot/core/HandlerChain.h"
#include <memory>
#include <vector>

namespace mergebot {
namespace sa {
void HandlerChain::chain() {
  assert(!this->Handlers_.empty() && this->Handlers_[0].get() &&
         "the handlers in Handler Chain cannot be empty");
  for (int cur = 0; cur + 1 < this->Handlers_.size(); ++cur) {
    this->Handlers_[cur]->setNext(this->Handlers_[cur + 1].get());
  }
}
} // namespace sa
} // namespace mergebot
