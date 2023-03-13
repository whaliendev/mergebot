//
// Created by whalien on 13/03/23.
//

#ifndef MB_CONFLICTBLOCK_H
#define MB_CONFLICTBLOCK_H
#include <string>
namespace mergebot {
namespace sa {
struct ConflictLines {
  /// start line of a conflict block
  int Start;
  /// end line of a conflict block
  int Offset;
};

struct ConflictBlock {
  int Index;
  /// our commit
  ConflictLines Ours;
  /// their commit
  ConflictLines Theirs;
  /// base commit
  ConflictLines Base;
};
} // namespace sa
} // namespace mergebot

#endif // MB_CONFLICTBLOCK_H
