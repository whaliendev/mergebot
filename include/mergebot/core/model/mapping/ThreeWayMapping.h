//
// Created by whalien on 15/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_CORE_MODEL_MAPPING_THREEWAYMAPPING_H
#define MB_INCLUDE_MERGEBOT_CORE_MODEL_MAPPING_THREEWAYMAPPING_H
#include <memory>
#include <optional>

#include "mergebot/core/model/SemanticNode.h"
namespace mergebot::sa {
struct ThreeWayMapping {
  ThreeWayMapping(std::optional<std::shared_ptr<SemanticNode>> OurNode,
                  std::optional<std::shared_ptr<SemanticNode>> BaseNode,
                  std::optional<std::shared_ptr<SemanticNode>> TheirNode)
      : OurNode(OurNode), BaseNode(BaseNode), TheirNode(TheirNode) {}

  std::optional<std::shared_ptr<SemanticNode>> OurNode;
  std::optional<std::shared_ptr<SemanticNode>> BaseNode;
  std::optional<std::shared_ptr<SemanticNode>> TheirNode;

  std::string toString() const {
    std::stringstream ss;
    const std::string OurQualifiedName =
        OurNode.has_value() ? OurNode.value()->QualifiedName : "(absent)";
    const std::string BaseQualifiedName =
        BaseNode.has_value() ? BaseNode.value()->QualifiedName : "(absent)";
    const std::string TheirQualifiedName =
        TheirNode.has_value() ? TheirNode.value()->QualifiedName : "(absent)";
    bool NameMatching = true;
    if (OurQualifiedName != BaseQualifiedName ||
        BaseQualifiedName != TheirQualifiedName) {
      NameMatching = false;
    }
    if (!NameMatching) {
      ss << "^^^^^^ ";
    }
    ss << fmt::format("ThreeWayMapping(OurNode={}, BaseNode={}, TheirNode={})",
                      OurQualifiedName, BaseQualifiedName, TheirQualifiedName);
    return ss.str();
  }

  friend std::ostream &operator<<(std::ostream &OS, const ThreeWayMapping &M) {
    OS << M.toString();
    return OS;
  }
};
} // namespace mergebot::sa

template <> struct fmt::formatter<mergebot::sa::ThreeWayMapping> {
  template <typename ParseContext> constexpr auto parse(ParseContext &ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const mergebot::sa::ThreeWayMapping &mapping,
              FormatContext &ctx) {
    return format_to(ctx.out(), "{}", mapping.toString());
  }
};

#endif // MB_INCLUDE_MERGEBOT_CORE_MODEL_MAPPING_THREEWAYMAPPING_H
