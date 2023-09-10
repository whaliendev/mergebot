//
// Created by whalien on 08/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_ACCESSSPECIFIERKIND_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_ACCESSSPECIFIERKIND_H
#include <cstdint>
namespace mergebot::sa {
enum class AccessSpecifierKind : uint8_t {
  None,
  Default,
  Public,
  Protected,
  Private,
};
}

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_ACCESSSPECIFIERKIND_H
