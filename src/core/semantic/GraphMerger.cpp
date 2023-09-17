//
// Created by whalien on 30/04/23.
//

#include "mergebot/core/semantic/GraphMerger.h"
#include "mergebot/core/semantic/GraphMatcher.h"
#include <oneapi/tbb/parallel_invoke.h>

namespace mergebot {
namespace sa {

void GraphMerger::threeWayMatch() {
  GraphMatcher OurMatcher(BaseGraph, OurGraph);
  GraphMatcher TheirMatcher(BaseGraph, TheirGraph);
  tbb::parallel_invoke([&]() { OurMatching = OurMatcher.match(); },
                       [&]() { TheirMatching = TheirMatcher.match(); });
}

void GraphMerger::threeWayMerge() {}
} // namespace sa
} // namespace mergebot