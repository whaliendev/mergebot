//
// Created by whalien on 13/03/23.
//

#ifndef MB_CONFLICTFILE_H
#define MB_CONFLICTFILE_H

#include "mergebot/utils/stringop.h"
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

#include "ConflictBlock.h"
namespace mergebot {
namespace sa {

struct ConflictFile {
  ConflictFile(std::string &&Filename,
               std::vector<ConflictBlock> &&ConflictBlocks)
      : Filename(Filename), ConflictBlocks(ConflictBlocks), Resolved(false) {}

  ConflictFile(std::string const &Filename,
               std::vector<ConflictBlock> const &ConflictBlocks)
      : Filename(Filename), ConflictBlocks(ConflictBlocks), Resolved(false) {}

  /// absolute file path
  std::string Filename;

  /// conflict blocks of a conflict file
  std::vector<ConflictBlock> ConflictBlocks;

  /// whether this file is resolved
  bool Resolved = false;

  explicit operator std::string() {
    return fmt::format(
        "ConflictFile(Filename = {}, ConflictBlocks = {}, Resolved = {})",
        Filename, fmt::join(ConflictBlocks, ", "), Resolved);
  }

  friend bool operator==(ConflictFile const &Lhs, ConflictFile const &Rhs) {
    return Lhs.Filename == Rhs.Filename &&
           Lhs.ConflictBlocks == Rhs.ConflictBlocks &&
           Lhs.Resolved == Rhs.Resolved;
  }
};
} // namespace sa
} // namespace mergebot
#endif // MB_CONFLICTFILE_H
