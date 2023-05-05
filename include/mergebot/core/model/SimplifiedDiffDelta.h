//
// Created by whalien on 04/05/23.
//

#ifndef MB_SIMPLIFIEDDIFFDELTA_H
#define MB_SIMPLIFIEDDIFFDELTA_H
#include "mergebot/utility.h"
#include <algorithm>
#include <fmt/core.h>
#include <magic_enum.hpp>
#include <sstream>
#include <string>
namespace mergebot {
namespace sa {
/// data class representing diff delta
struct SimplifiedDiffDelta {
  enum DeltaType {
    UNMODIFIED = 0, /**< no changes */
    ADDED = 1,      /**< entry does not exist in old version */
    DELETED = 2,    /**< entry does not exist in new version */
    MODIFIED = 3,   /**< entry content changed between old and new */
    RENAMED = 4,    /**< entry was renamed between old and new */
    COPIED = 5,     /**< entry was copied from another old entry */
    IGNORED = 6,    /**< entry is ignored item in workdir */
    UNTRACKED = 7,  /**< entry is untracked item in workdir */
    TYPECHANGE = 8, /**< type of entry changed between old and new */
    UNREADABLE = 9, /**< entry is unreadable */
    CONFLICTED = 10 /**< entry in the index is conflicted */
  };
  std::string OldPath;
  std::string NewPath;
  DeltaType Type;
  uint Similarity; /**< for RENAMED and COPIED, value 0-100 */

  double score() const { return Similarity / 100.0; }

  std::string toString() const {
    std::ostringstream OSS;
    OSS << *this;
    return OSS.str();
  }

  //  SimplifiedDiffDelta &operator=(SimplifiedDiffDelta const &Other) {
  //    if (this == &Other) {
  //      return *this;
  //    }
  //    this->OldPath = Other.OldPath;
  //    this->NewPath = Other.NewPath;
  //    this->Type = Other.Type;
  //    this->Similarity = Other.Similarity;
  //    return *this;
  //  }

  friend std::ostream &operator<<(std::ostream &OS,
                                  SimplifiedDiffDelta const &SDD) {
    return OS << fmt::format("SimplifiedDiffDelta(oldPath={}, newPath={}, "
                             "deltaType={}, similarity={})",
                             SDD.OldPath, SDD.NewPath, SDD.Type,
                             SDD.Similarity);
  }

  bool operator==(SimplifiedDiffDelta const &SDD) const noexcept {
    return toString() == SDD.toString();
  }
};
} // namespace sa
} // namespace mergebot

namespace std {
template <> struct hash<mergebot::sa::SimplifiedDiffDelta> {
  size_t
  operator()(mergebot::sa::SimplifiedDiffDelta const &SDD) const noexcept {
    size_t H = 1;
    mergebot::hash_combine(H, SDD.OldPath);
    mergebot::hash_combine(H, SDD.NewPath);
    mergebot::hash_combine(H, SDD.Type);
    mergebot::hash_combine(H, SDD.Similarity);
    return H;
  }
};
} // namespace std
#endif // MB_SIMPLIFIEDDIFFDELTA_H
