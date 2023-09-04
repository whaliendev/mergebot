//
// Created by whalien on 08/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_EDGETYPE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_EDGETYPE_H

#include <array>
#include <cstdint>
namespace mergebot {
namespace sa {
enum class EdgeType : uint8_t {
  /**
   * file and folder level
   */
  CONTAIN, // isStructuralEdge
  INCLUDE,
  INHERIT,
  /**
   * inside file
   */
  DECLARE,
  DEFINE,
  INITIALIZE,
  READ,
  WRITE,
  CALL,
  COUNT
};

struct EdgeTypeInfo {
  EdgeType type;
  bool isStructuralEdge;
  const char *label;

  constexpr EdgeTypeInfo(EdgeType type, bool isStructuralEdge,
                         const char *label)
      : type(type), isStructuralEdge(isStructuralEdge), label(label) {}

  constexpr const char *toPrettyString() const { return label; }
};

// Preallocate EdgeTypeInfo instances at compile-time in a fixed-size array
constexpr std::array<EdgeTypeInfo, static_cast<std::uint8_t>(EdgeType::COUNT)>
    EDGE_TYPE_INFO_ARRAY = {
        EdgeTypeInfo(EdgeType::CONTAIN, true, "contains"),
        EdgeTypeInfo(EdgeType::INCLUDE, false, "includes"),
        EdgeTypeInfo(EdgeType::INHERIT, false, "inherits"),
        EdgeTypeInfo(EdgeType::DECLARE, false, "declares"),
        EdgeTypeInfo(EdgeType::DEFINE, false, "defines"),
        EdgeTypeInfo(EdgeType::INITIALIZE, false, "initializes"),
        EdgeTypeInfo(EdgeType::READ, false, "reads"),
        EdgeTypeInfo(EdgeType::WRITE, false, "writes"),
        EdgeTypeInfo(EdgeType::CALL, false, "calls")};

// Access EdgeTypeInfo using array indexing
constexpr const EdgeTypeInfo &getEdgeTypeInfo(EdgeType type) {
  return EDGE_TYPE_INFO_ARRAY[static_cast<std::uint8_t>(type)];
}

} // namespace sa
} // namespace mergebot

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_EDGETYPE_H
