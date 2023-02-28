//
// Created by whalien on 25/02/23.
//

#ifndef MB_NONCOPYABLE_H
#define MB_NONCOPYABLE_H
namespace mergebot {

class noncopyable {
 public:
  noncopyable(const noncopyable&) = delete;
  void operator=(const noncopyable&) = delete;

 protected:
  noncopyable() = default;
  ~noncopyable() = default;
};

}  // namespace mergebot
#endif  // MB_NONCOPYABLE_H
