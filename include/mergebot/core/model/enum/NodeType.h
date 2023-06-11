//
// Created by whalien on 08/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_NODETYPE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_NODETYPE_H

#include <cstdint>
namespace mergebot {
namespace sa {
enum class NodeType : std::uint16_t {
  PROJECT = 0,
  TRANSLATION_UNIT = 1,
  NAMESPACE = 2,
};
}
} // namespace mergebot

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_NODETYPE_H
