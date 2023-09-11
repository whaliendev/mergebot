//
// Created by whalien on 08/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_EDGEKIND_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_EDGEKIND_H

#include <array>
#include <cstdint>
namespace mergebot {
namespace sa {
enum class EdgeKind : uint8_t {
  ILLEGAL,
  /**
   * file and folder level
   */
  CONTAIN, // isStructuralEdge
  INCLUDE,
  //  INHERIT,
  REFERENCE,
  DEFINE,
  USE
  /**
   * inside file
   */
  //  DECLARE,
  //  DEFINE,
  //  INITIALIZE,
  //  READ,
  //  WRITE,
  //  CALL,
  //  COUNT
};

struct EdgeTypeInfo {
  EdgeKind type;
  bool isStructuralEdge;
  const char *label;

  constexpr EdgeTypeInfo(EdgeKind type, bool isStructuralEdge,
                         const char *label)
      : type(type), isStructuralEdge(isStructuralEdge), label(label) {}

  constexpr const char *toPrettyString() const { return label; }
};

// Preallocate EdgeTypeInfo instances at compile-time in a fixed-size array
// constexpr std::array<EdgeTypeInfo,
// static_cast<std::uint8_t>(EdgeKind::COUNT)>
//    EDGE_TYPE_INFO_ARRAY = {
//        EdgeTypeInfo(EdgeKind::CONTAIN, true, "contains"),
//        EdgeTypeInfo(EdgeKind::INCLUDE, false, "includes"),
//        EdgeTypeInfo(EdgeKind::INHERIT, false, "inherits"),
//        EdgeTypeInfo(EdgeKind::DECLARE, false, "declares"),
//        EdgeTypeInfo(EdgeKind::DEFINE, false, "defines"),
//        EdgeTypeInfo(EdgeKind::INITIALIZE, false, "initializes"),
//        EdgeTypeInfo(EdgeKind::READ, false, "reads"),
//        EdgeTypeInfo(EdgeKind::WRITE, false, "writes"),
//        EdgeTypeInfo(EdgeKind::CALL, false, "calls")};

// Access EdgeTypeInfo using array indexing
// constexpr const EdgeTypeInfo &getEdgeTypeInfo(EdgeKind type) {
//  return EDGE_TYPE_INFO_ARRAY[static_cast<std::uint8_t>(type)];
//}

} // namespace sa
} // namespace mergebot

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_EDGEKIND_H
