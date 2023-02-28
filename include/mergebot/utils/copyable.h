//
// Created by whalien on 25/02/23.
//

#ifndef MB_COPYABLE_H
#define MB_COPYABLE_H
namespace mergebot {
/// A tag class emphasises the objects are copyable.
/// The empty base class optimization applies.
/// Any derived class of copyable should be a value type.
class copyable {
 protected:
  copyable() = default;
  ~copyable() = default;
};
}  // namespace mergebot
#endif  // MB_COPYABLE_H
