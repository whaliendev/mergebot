//
// Created by whalien on 04/09/23.
//

#include "mergebot/parser/utils.h"

#include <re2/re2.h>

#include <cassert>
#include <sstream>
#include <string>

#include "mergebot/parser/node.h"
#include "mergebot/utils/stringop.h"

namespace mergebot {
namespace ts {
std::pair<size_t, std::string> getTranslationUnitComment(const ts::Node &root) {
  std::stringstream comment;
  size_t commentCnt = 0;
  for (ts::Node const &child : root.children) {
    if (child.isNamed() && child.type() == ts::cpp::symbols::sym_comment.name) {
      comment << child.text() << "\n";
      commentCnt++;
    } else {
      break;
    }
  }
  return {commentCnt, comment.str()};
}

std::string getNodeComment(const Node &node) {
  std::stringstream comment;
  std::optional<ts::Node> previous = node.prevSibling();
  if (!previous.has_value() || !previous.value().isNamed() ||
      previous.value().type() != ts::cpp::symbols::sym_comment.name) {
    return "";
  }
  std::vector<std::string> previousComments;
  while (previous.has_value() && previous.value().isNamed() &&
         previous.value().type() == ts::cpp::symbols::sym_comment.name) {
    previousComments.push_back(previous.value().text());
    previous = previous.value().prevSibling();
  }

  for (auto it = previousComments.rbegin(); it != previousComments.rend();
       ++it) {
    comment << *it << "\n";
  }

  return comment.str();
}

int nextSiblingDistance(const Node &node, bool named /* = false */) {
  int dis = -1;
  std::optional<ts::Node> next =
      named ? node.nextNamedSibling() : node.nextSibling();
  if (next.has_value()) {
    dis = next.value().startPoint().row - node.endPoint().row - 1;
  }
  return dis;
}

size_t beforeFirstChildEOLs(const ts::Node &node) {
  auto bodyNodeOpt = node.getChildByFieldName(ts::cpp::fields::field_body.name);
  assert(bodyNodeOpt.has_value() &&
         "invariant: body node of node should exist");
  const ts::Node bodyNode = bodyNodeOpt.value();
  return bodyNode.startPoint().row - node.startPoint().row - 1;
}

std::pair<bool, std::vector<std::string>> getHeaderGuard(
    const Node &TUNode, size_t &BeforeBodyChildCnt, ts::Node &TURoot) {
  assert(TUNode.type() == ts::cpp::symbols::sym_translation_unit.name &&
         "invariant: node should be a translation unit");
  const ts::Node &HeaderGuardNode = TUNode.children[BeforeBodyChildCnt];
  if (HeaderGuardNode.type() == ts::cpp::symbols::sym_preproc_call.name) {
    RE2 pattern(R"(^\s*#\s*pragma\s+once\s*$)");
    if (RE2::FullMatch(HeaderGuardNode.text(), pattern)) {
      BeforeBodyChildCnt += 1;
      return std::pair<bool, std::vector<std::string>>(
          false, {HeaderGuardNode.text()});
    }
  }
  if (HeaderGuardNode.type() == ts::cpp::symbols::sym_preproc_ifdef.name) {
    size_t ChildrenCnt = HeaderGuardNode.namedChildrenCount();
    if (ChildrenCnt < 2) {
      return {false, {}};
    }
    const ts::Node defineNode = HeaderGuardNode.namedChildren()[1];
    if (defineNode.type() == ts::cpp::symbols::sym_preproc_def.name) {
      TURoot = HeaderGuardNode;
      BeforeBodyChildCnt = 2;  // first two is identifier and define
      size_t first_newline = HeaderGuardNode.text().find('\n');
      std::string ifndefText = HeaderGuardNode.text().substr(0, first_newline);
      // TODO(hwa): validate it's a #endif
      std::string endifText = HeaderGuardNode.text().substr(
          HeaderGuardNode.text().rfind('\n') + 1, std::string::npos);
      return {true, {ifndefText, defineNode.text(), endifText}};
    }
  }
  return {false, {}};
}

std::vector<std::pair<ts::Point, std::string>> getFrontDecls(
    const ts::Node &TURoot, size_t &cnt) {
  const size_t childrenCnt = TURoot.childrenCount();
  if (cnt >= childrenCnt) {
    return {};
  }

  std::vector<std::pair<ts::Point, std::string>> ret;
  for (ts::Node node = TURoot.children[cnt]; cnt < childrenCnt;
       node = TURoot.children[cnt]) {
    if (util::starts_with(node.type(), "preproc") ||
        node.type() == ts::cpp::symbols::sym_using_declaration.name ||
        node.type() == ts::cpp::symbols::sym_alias_declaration.name ||
        node.type() == ts::cpp::symbols::sym_type_definition.name ||
        node.type() == ts::cpp::symbols::sym_linkage_specification.name ||
        isTypeDecl(node)) {
      std::string textualContent =
          node.endPoint().row - node.startPoint().row
              ? std::string(util::string_trim(node.text()))
              : node.text();
      ret.push_back({node.startPoint(), textualContent});
      ++cnt;
    } else if (node.type() == ts::cpp::symbols::sym_comment.name) {
      size_t commentCnt = 0;
      auto [orphan, comment] = getComment(node, commentCnt);
      ret.push_back({node.startPoint(), comment});
      cnt += commentCnt;
    } else {
      break;
    }
  }

  return ret;
}

bool isTypeDecl(const Node &node) {
  bool isTypeSpecifier =
      node.type() == ts::cpp::symbols::sym_class_specifier.name ||
      node.type() == ts::cpp::symbols::sym_struct_specifier.name ||
      node.type() == ts::cpp::symbols::sym_union_specifier.name ||
      node.type() == ts::cpp::symbols::sym_enum_specifier.name;

  std::optional<ts::Node> bodyNodeOpt =
      node.getChildByFieldName(ts::cpp::fields::field_body.name);
  return isTypeSpecifier && !bodyNodeOpt.has_value();
}

std::pair<bool, std::string> getComment(const Node &commentNode,
                                        size_t &commentCnt) {
  assert(commentNode.type() == ts::cpp::symbols::sym_comment.name &&
         "invariant: node should be a comment");
  ts::Node prevComment = commentNode;
  ts::Node curComment = commentNode;
  std::stringstream ret;
  while (curComment.type() == ts::cpp::symbols::sym_comment.name) {
    ret << curComment.text() << "\n";
    commentCnt++;
    std::optional<ts::Node> siblingOpt = curComment.nextSibling();
    if (!siblingOpt.has_value()) {
      return {true, ret.str()};
    }
    prevComment = curComment;
    curComment = siblingOpt.value();
  }
  if (curComment.startPoint().row - prevComment.startPoint().row > 1) {
    return {true, ret.str()};
  } else {
    return {false, ret.str()};
  }
}
}  // namespace ts
}  // namespace mergebot