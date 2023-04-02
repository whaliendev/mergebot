//
// Created by whalien on 31/03/23.
//

#ifndef MB_CONFLICTMARK_H
#define MB_CONFLICTMARK_H
namespace mergebot {
namespace sa {
enum class ConflictMark : int {
  OURS,   // <<<<<<<
  BASE,   // |||||||
  THEIRS, // =======
  END     // >>>>>>>
};
}
} // namespace mergebot
#endif // MB_CONFLICTMARK_H
