//
// Created by whalien on 03/05/23.
//

#ifndef MB_SIDE_H
#define MB_SIDE_H
namespace mergebot {
namespace sa {
enum class Side : int {
  OURS,   // "ours"
  BASE,   // "base"
  THEIRS, // "theirs"
  OUT,    // "out"
};
}
} // namespace mergebot

#endif // MB_SIDE_H
