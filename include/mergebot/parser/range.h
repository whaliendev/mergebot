//
// Created by whalien on 07/05/23.
//

#ifndef MB_RANGE_H
#define MB_RANGE_H

#include <tree_sitter/api.h>

#include <ostream>

namespace mergebot {
namespace ts {

using Range = TSRange;

}  // namespace ts
}  // namespace mergebot

extern "C" {
bool operator==(mergebot::ts::Range const &lhs, mergebot::ts::Range const &rhs);
bool operator!=(mergebot::ts::Range const &lhs, mergebot::ts::Range const &rhs);
}

extern std::ostream &operator<<(std::ostream &, mergebot::ts::Range const &);

#endif  // MB_RANGE_H
