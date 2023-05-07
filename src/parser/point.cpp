//
// Created by whalien on 07/05/23.
//

#include "mergebot/parser/point.h"

std::ostream& operator<<(std::ostream& os, mergebot::ts::Point const& p) {
  return os << "Point(" << p.row << ", " << p.column << ')';
}