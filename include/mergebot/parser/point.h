//
// Created by whalien on 07/05/23.
//

#ifndef MB_POINT_H
#define MB_POINT_H

#include <tree_sitter/api.h>

#include <iostream>
namespace mergebot {
namespace ts {

using Point = TSPoint;

}  // namespace ts
}  // namespace mergebot
extern std::ostream& operator<<(std::ostream&, mergebot::ts::Point const&);

#endif  // MB_POINT_H
