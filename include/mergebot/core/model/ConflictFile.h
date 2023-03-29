//
// Created by whalien on 13/03/23.
//

#ifndef MB_CONFLICTFILE_H
#define MB_CONFLICTFILE_H

#include "mergebot/utils/stringop.h"
#include <string>
#include <vector>

#include "ConflictBlock.h"
namespace mergebot {
namespace sa {
struct ConflictFile {
  ConflictFile(std::string &&Filename,
               std::vector<ConflictBlock> &&ConflictBlocks)
      : Filename(Filename), ConflictBlocks(ConflictBlocks) {}

  ConflictFile(std::string const &Filename,
               std::vector<ConflictBlock> const &ConflictBlocks)
      : Filename(Filename), ConflictBlocks(ConflictBlocks) {}

  /// absolute file path
  std::string Filename;

  /// conflict blocks of a conflict file
  std::vector<ConflictBlock> ConflictBlocks;
  explicit operator std::string() {
    return fmt::format("ConflictFile(Filename = {}, ConflictBlocks = {})",
                       Filename,
                       mergebot::util::string_join(ConflictBlocks, ", "));
  }

  friend bool operator==(ConflictFile const &Lhs, ConflictFile const &Rhs) {
    return Lhs.Filename == Rhs.Filename &&
           Lhs.ConflictBlocks == Rhs.ConflictBlocks;
  }
};
} // namespace sa
} // namespace mergebot
#endif // MB_CONFLICTFILE_H
