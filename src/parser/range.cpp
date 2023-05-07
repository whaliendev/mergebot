//
// Created by whalien on 07/05/23.
//

#include "mergebot/parser/range.h"

#include "mergebot/parser/point.h"

using mergebot::ts::Range;

bool operator==(Range const& a, Range const& b) {
  return (
      (a.start_point.row == b.start_point.row) &&
      (a.start_point.column == b.start_point.column) &&
      (a.start_byte == b.start_byte) && (a.end_point.row == b.end_point.row) &&
      (a.end_point.column == b.end_point.column) && (a.end_byte == b.end_byte));
}

bool operator!=(Range const& a, Range const& b) { return !(a == b); }

std::ostream& operator<<(std::ostream& os, Range const& r) {
  return os << "Range(start_point=" << r.start_point
            << ", start_byte=" << r.start_byte << ", end_point=" << r.end_point
            << ", end_byte" << r.end_byte << ')';
}