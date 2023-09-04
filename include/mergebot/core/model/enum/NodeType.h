//
// Created by whalien on 08/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_NODETYPE_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_NODETYPE_H

#include <array>
#include <cstdint>
#include <string_view>

namespace mergebot {
namespace sa {

/**
 * @enum NodeType
 * @brief Enumerates the types of nodes that can appear in a C++ code structure.
 */
enum class NodeType : std::uint16_t {
  /// logical nodes
  /// @brief Represents a project.
  /// Example: A folder containing multiple C++ source and header files.
  PROJECT,

  /// @brief Represents a translation unit, typically a single source file.
  /// Example: A single `.cpp` file.
  TRANSLATION_UNIT,

  /// physical nodes
  //// non-terminal nodes

  /// @brief Represents a linkage specification.
  /// Example:
  /// @code
  /// #ifndef cpp
  /// #define cpp
  /// ...
  /// #endif
  /// @endcode
  IFDEF_BLOCK,

  /// @brief Represents a linkage specification.
  /// Example:
  /// @code
  /// extern "C" void doSomething();
  ///
  /// extern "C" {
  ///   // ...
  /// }
  /// @endcode
  LINKAGE_SPEC,

  /// @brief Represents a namespace.
  /// Example:
  /// @code
  /// namespace MyNamespace {
  ///   // ...
  /// }
  /// @endcode
  NAMESPACE,

  /// Note that class, struct and union can be templated while other
  /// non-terminal node not.
  /// @brief Represents a class.
  /// Example:
  /// @code
  /// class MyClass {
  ///   // ...
  /// };
  /// @endcode
  CLASS,

  /// @brief Represents a struct.
  /// Example:
  /// @code
  /// struct MyStruct {
  ///   // ...
  /// };
  /// @endcode
  STRUCT,

  /// @brief Represents a union.
  /// Example:
  /// @code
  /// union MyUnion {
  ///   // ...
  /// };
  /// @endcode
  UNION,

  /// @brief Represents an enum.
  /// Example:
  /// @code
  /// enum MyEnum {
  ///   // ...
  /// };
  /// @endcode
  ENUM,

  //// terminal nodes

  /// @brief Represents a function.
  /// Example:
  /// @code
  /// void myFunction() {
  ///   // ...
  /// }
  /// @endcode
  FUNCTION,

  /// @brief Represents a variable.
  /// Example:
  /// @code
  /// int myVariable;
  /// @endcode
  FIELD,

  /// @brief Represents an enum constant.
  /// Example:
  /// @code
  /// enum MyEnum {
  ///   MY_CONSTANT  // Enum constant
  /// };
  /// @endcode
  ENUM_CONSTANT,

  /// @brief Represents a constructor.
  /// Example:
  /// @code
  /// class MyClass {
  /// public:
  ///   MyClass() {
  ///     // Constructor body
  ///   }
  /// };
  /// @endcode
  CONSTRUCTOR,

  //  /// @brief Represents a destructor.
  //  /// Example:
  //  /// @code
  //  /// class MyClass {
  //  /// public:
  //  ///   ~MyClass() {
  //  ///     // Destructor body
  //  ///   }
  //  /// };
  //  /// @endcode
  //  DESTRUCTOR,

  /// @brief Represents a friend function.
  /// Example:
  /// @code
  /// class MyClass {
  ///   friend void myFriendFunction(MyClass&);
  /// };
  /// @endcode
  FRIEND_FUNCTION,

  /// @brief Represents a friend class.
  /// Example:
  /// @code
  /// class MyClass {
  ///   friend class MyFriendClass;
  /// };
  /// @endcode
  FRIEND_CLASS,

  /// @brief Represents a using directive or declaration.
  /// Example:
  /// @code
  /// using namespace std;
  /// @endcode
  USING,

  /// @brief Represents a typedef.
  /// Example:
  /// @code
  /// typedef int MyInt;
  /// @endcode
  TYPEDEF,

  /// @brief Represents a comment that is not associated with any code element.
  /// Example:
  /// @code
  /// // This is an orphan comment
  /// @endcode
  ORPHAN_COMMENT,

  /// @brief Represents all other unknown textual node or error node
  TEXTUAL,

  /// @brief Special value to get the number of enum items.
  COUNT
};

struct NodeTypeInfo {
  NodeType type;
  int level;
  const char *label;

  constexpr NodeTypeInfo(NodeType type, int level, const char *label)
      : type(type), level(level), label(label) {}

  constexpr std::string_view toPrettyString() const { return label; }
};

// Preallocate NodeTypeInfo instances at compile-time in a fixed-size array
constexpr std::array<NodeTypeInfo, static_cast<std::uint16_t>(NodeType::COUNT)>
    NODE_TYPE_INFO_ARRAY = {
        NodeTypeInfo(NodeType::PROJECT, 0, "project"),
        NodeTypeInfo(NodeType::TRANSLATION_UNIT, 1, "translation_unit"),
        NodeTypeInfo(NodeType::IFDEF_BLOCK, 2, "ifdef_block"),
        NodeTypeInfo(NodeType::LINKAGE_SPEC, 2, "linkage_spec"),
        NodeTypeInfo(NodeType::NAMESPACE, 2, "namespace"),
        NodeTypeInfo(NodeType::CLASS, 3, "class"),
        NodeTypeInfo(NodeType::STRUCT, 3, "struct"),
        NodeTypeInfo(NodeType::UNION, 3, "union"),
        NodeTypeInfo(NodeType::ENUM, 3, "enum"),
        NodeTypeInfo(NodeType::FUNCTION, 3, "function"),
        NodeTypeInfo(NodeType::FIELD, 3, "field"),
        NodeTypeInfo(NodeType::ENUM_CONSTANT, 3, "enum_constant"),
        NodeTypeInfo(NodeType::CONSTRUCTOR, 3, "constructor"),
        //        NodeTypeInfo(NodeType::DESTRUCTOR, 3, "destructor"),
        NodeTypeInfo(NodeType::FRIEND_FUNCTION, 3, "friend_function"),
        NodeTypeInfo(NodeType::FRIEND_CLASS, 3, "friend_class"),
        NodeTypeInfo(NodeType::USING, 3, "using"),
        NodeTypeInfo(NodeType::TYPEDEF, 3, "typedef"),
        NodeTypeInfo(NodeType::TEXTUAL, 3, "unknown"),
        NodeTypeInfo(NodeType::ORPHAN_COMMENT, 4, "orphan_comment")};

// Access NodeTypeInfo using array indexing
constexpr const NodeTypeInfo &getNodeTypeInfo(NodeType type) {
  return NODE_TYPE_INFO_ARRAY[static_cast<std::uint16_t>(type)];
}

constexpr bool isCompositeNode(NodeType type) {
  return getNodeTypeInfo(type).level >=
             getNodeTypeInfo(NodeType::IFDEF_BLOCK).level &&
         getNodeTypeInfo(type).level <
             getNodeTypeInfo(NodeType::FUNCTION).level;
}

constexpr bool isTerminalNode(NodeType type) {
  return getNodeTypeInfo(type).level >=
             getNodeTypeInfo(NodeType::FUNCTION).level &&
         getNodeTypeInfo(type).level < getNodeTypeInfo(NodeType::COUNT).level;
}
} // namespace sa
} // namespace mergebot

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_ENUM_NODETYPE_H
