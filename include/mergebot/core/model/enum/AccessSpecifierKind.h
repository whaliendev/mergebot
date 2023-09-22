//
// Created by whalien on 08/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_ACCESSSPECIFIERKIND_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_ACCESSSPECIFIERKIND_H
#include <cstdint>
#include <magic_enum.hpp>
namespace mergebot::sa {
enum class AccessSpecifierKind : uint8_t {
  None,
  Default,
  Public,
  Protected,
  Private,
};
}

namespace magic_enum {
namespace customize {
template <>
constexpr customize_t enum_name<mergebot::sa::AccessSpecifierKind>(
    mergebot::sa::AccessSpecifierKind Kind) noexcept {
  using AccessSpecifierKind = mergebot::sa::AccessSpecifierKind;
  switch (Kind) {
  case AccessSpecifierKind::None:
    return default_tag;
  case AccessSpecifierKind::Default:
    return default_tag;
  case AccessSpecifierKind::Public:
    return "public:";
  case AccessSpecifierKind::Protected:
    return "protected:";
  case AccessSpecifierKind::Private:
    return "private:";
  }
  return default_tag;
}
} // namespace customize
} // namespace magic_enum

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_ACCESSSPECIFIERKIND_H
