//
// Created by whalien on 04/05/23.
//

#ifndef MB_SIMPLIFIEDDIFFDELTA_H
#define MB_SIMPLIFIEDDIFFDELTA_H
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
  std::string oldPath;
  std::string newPath;
  DeltaType deltaType;
  uint similarity; /**< for RENAMED and COPIED, value 0-100 */

  double score() const { return similarity / 100.0; }

  std::string toString() const {
    std::ostringstream oss;
    oss << *this;
    return oss.str();
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  SimplifiedDiffDelta const &sdd) {
    return os << fmt::format("SimplifiedDiffDelta(oldPath={}, newPath={}, "
                             "deltaType={}, similarity={})",
                             sdd.oldPath, sdd.newPath, sdd.deltaType,
                             sdd.similarity);
  }
};
} // namespace sa
} // namespace mergebot
#endif // MB_SIMPLIFIEDDIFFDELTA_H
