//
// Created by whalien on 13/03/23.
//

#ifndef MB_CONFLICTFILE_H
#define MB_CONFLICTFILE_H

#include <string>
#include <vector>

#include "ConflictBlock.h"
namespace mergebot {
namespace sa {
struct ConflictFile {
  /// canonical filename with project path stripped
  /// e.g.:
  /// canonical filename:
  /// /home/whalien/codebase/aosp/frameworks/av/camera/cameraserver/main_cameraserver.cpp
  /// Filename: camera/cameraserver/main_cameraserver.cpp
  std::string Filename;

  /// conflict blocks of a conflict file
  // TODO: consider using SmallVector to optimize
  std::vector<ConflictBlock> ConflictBlocks;
};
} // namespace sa
} // namespace mergebot
#endif // MB_CONFLICTFILE_H
