//
// Created by whalien on 08/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_EDGETYPE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_EDGETYPE_H

#include <cstdint>
namespace mergebot {
namespace sa {
enum class EdgeType : uint8_t {
  /**
   * file and folder level
   */
  CONTAIN,
  INCLUDE,
  INHERIT,
  /**
   * inside file
   */
  DECLARE,
  DEFINE,
  READ,
  WRITE,
  CALL
};
}
} // namespace mergebot

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_EDGETYPE_H
