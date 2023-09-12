//
// Created by whalien on 08/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_NodeKind_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_NodeKind_H

#include <array>
#include <cstdint>
#include <string_view>

namespace mergebot {
namespace sa {

/**
 * @enum NodeKind
 * @brief Enumerates the types of nodes that can appear in a C++ code structure.
 */
enum class NodeKind : std::uint16_t {
  // clang-format off
  NODE,
    COMPOSITE_NODE,
      TRANSLATION_UNIT,
      LINKAGE_SPEC_LIST,
      NAMESPACE,
      TYPE,
      ENUM,
      LAST_COMPOSITE_NODE,

    TERMINAL_NODE,
      FIELD_DECLARATION,
      FUNC_DEF,
      FUNC_OPERATOR_CAST,
      FUNC_SPECIAL_MEMBER,
      ORPHAN_COMMENT,
      TEXTUAL,
      ACCESS_SPECIFIER,
      LAST_TERMINAL_NODE,

    COUNT,
  // clang-format on
};

// struct NodeKindInfo {
//   NodeKind type;
//   int level;
//   const char *label;
//
//   constexpr NodeKindInfo(NodeKind type, int level, const char *label)
//       : type(type), level(level), label(label) {}
//
//   constexpr std::string_view toPrettyString() const { return label; }
// };

// Preallocate NodeKindInfo instances at compile-time in a fixed-size array
// constexpr std::array<NodeKindInfo,
// static_cast<std::uint16_t>(NodeKind::COUNT)>
//    NODE_TYPE_INFO_ARRAY = {
//        NodeKindInfo(NodeKind::PROJECT, 0, "project"),
//        NodeKindInfo(NodeKind::TRANSLATION_UNIT, 1, "translation_unit"),
//        //        NodeKindInfo(NodeKind::IFDEF_BLOCK, 2, "ifdef_block"),
//        NodeKindInfo(NodeKind::LINKAGE_SPEC_LIST, 2, "linkage_spec"),
//        NodeKindInfo(NodeKind::NAMESPACE, 2, "namespace"),
//        NodeKindInfo(NodeKind::Type, 3, "class | struct | union"),
//        //        NodeKindInfo(NodeKind::CLASS, 3, "class"),
//        //        NodeKindInfo(NodeKind::STRUCT, 3, "struct"),
//        //        NodeKindInfo(NodeKind::UNION, 3, "union"),
//        NodeKindInfo(NodeKind::ENUM, 3, "enum"),
//        NodeKindInfo(NodeKind::FUNCTION, 3, "function"),
//        NodeKindInfo(NodeKind::FIELD_DECLARATION, 3, "field"),
//        //        NodeKindInfo(NodeKind::ENUM_CONSTANT, 3, "enum_constant"),
//        NodeKindInfo(NodeKind::CONSTRUCTOR, 3, "constructor"),
//        //        NodeKindInfo(NodeKind::DESTRUCTOR, 3, "destructor"),
//        //        NodeKindInfo(NodeKind::FRIEND_FUNCTION, 3,
//        "friend_function"),
//        //        NodeKindInfo(NodeKind::FRIEND_CLASS, 3, "friend_class"),
//        //        NodeKindInfo(NodeKind::USING, 3, "using"),
//        //        NodeKindInfo(NodeKind::TYPEDEF, 3, "typedef"),
//        NodeKindInfo(NodeKind::TEXTUAL, 3, "unknown"),
//        NodeKindInfo(NodeKind::ORPHAN_COMMENT, 4, "orphan_comment")};
//
//// Access NodeKindInfo using array indexing
// constexpr const NodeKindInfo &getNodeKindInfo(NodeKind type) {
//   return NODE_TYPE_INFO_ARRAY[static_cast<std::uint16_t>(type)];
// }

// constexpr bool isCompositeNode(NodeKind type) {
//   return getNodeKindInfo(type).level >=
//              getNodeKindInfo(NodeKind::TRANSLATION_UNIT).level &&
//          getNodeKindInfo(type).level <
//              getNodeKindInfo(NodeKind::FUNCTION).level;
// }
//
// constexpr bool isTerminalNode(NodeKind type) {
//   return getNodeKindInfo(type).level >=
//              getNodeKindInfo(NodeKind::FUNCTION).level &&
//          getNodeKindInfo(type).level <
//          getNodeKindInfo(NodeKind::COUNT).level;
// }
} // namespace sa
} // namespace mergebot

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_NodeKind_H
