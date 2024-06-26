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
  if (!previous.has_value() ||
      previous.value().type() != ts::cpp::symbols::sym_comment.name) {
    return "";
  }
  ts::Node mostPreviousComment = node;
  std::vector<std::string> previousComments;
  while (previous.has_value() &&
         previous.value().type() == ts::cpp::symbols::sym_comment.name) {
    mostPreviousComment = previous.value();
    previousComments.push_back(mostPreviousComment.text());
    previous = previous.value().prevSibling();
  }

  // fix for inline comment
  if (previous.has_value()) {
    if (mostPreviousComment.startPoint().row ==
        previous.value().endPoint().row) {
      return "";
    }
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
    dis = next.value().startPoint().row - node.endPoint().row;
  }
  return dis;
}

int beforeFirstChildEOLs(const ts::Node &node) {
  auto bodyNodeOpt = node.getChildByFieldName(ts::cpp::fields::field_body.name);
  assert(bodyNodeOpt.has_value() &&
         "invariant: body node of node should exist");
  const ts::Node bodyNode = bodyNodeOpt.value();
  int offset = bodyNode.startPoint().row - node.endPoint().row;
  if (bodyNode.namedChildren().size() > 0) {
    const ts::Node firstChild = bodyNode.namedChildren()[0];
    // as node.endPoint exceeds firstChild.startPoint
    offset = firstChild.startPoint().row - node.startPoint().row;
  }
  return offset < 0 ? 0 : offset;
}

std::pair<bool, std::vector<std::string>> getHeaderGuard(
    const Node &TUNode, size_t &BeforeBodyChildCnt, ts::Node &TURoot) {
  assert(TUNode.type() == ts::cpp::symbols::sym_translation_unit.name &&
         "invariant: node should be a translation unit");
  const ts::Node &HeaderGuardNode = TUNode.children[BeforeBodyChildCnt];
  if (HeaderGuardNode.type() == ts::cpp::symbols::sym_preproc_call.name) {
    RE2 pattern(R"(\s*#\s*pragma\s+once\s*)");
    if (RE2::PartialMatch(HeaderGuardNode.text(), pattern)) {
      BeforeBodyChildCnt += 1;
      return std::pair<bool, std::vector<std::string>>(
          false, {HeaderGuardNode.text()});
    }
  }
  if (HeaderGuardNode.type() == ts::cpp::symbols::sym_preproc_ifdef.name) {
    size_t ChildrenCnt =
        HeaderGuardNode.namedChildrenCount();  // 检查是否为传统的Header Guard
    if (ChildrenCnt < 2) {
      return {false, {}};
    }
    const ts::Node defineNode = HeaderGuardNode.namedChildren()[1];
    if (defineNode.type() == ts::cpp::symbols::sym_preproc_def.name) {
      TURoot = HeaderGuardNode;    // 更改TURoot为ifdef block
      BeforeBodyChildCnt = 2 + 1;  // first two is identifier and define
      size_t first_newline = HeaderGuardNode.text().find('\n');
      std::string ifndefText = HeaderGuardNode.text().substr(0, first_newline);
      std::string endifText = HeaderGuardNode.text().substr(
          HeaderGuardNode.text().rfind('\n') + 1, std::string::npos);
      if (HeaderGuardNode.nextSibling()) {
        const ts::Node nextSibling = HeaderGuardNode.nextSibling().value();
        if (nextSibling.type() == ts::cpp::symbols::sym_comment.name) {
          endifText += nextSibling.text();
        }
      }
      return {true, {ifndefText, defineNode.text(), endifText}};
    }
  }
  return {false, {}};
}

std::pair<std::vector<std::string>, size_t> getFrontDecls(
    const ts::Node &TURoot, size_t &cnt) {
  const size_t childrenCnt = TURoot.childrenCount();
  if (cnt >= childrenCnt) {
    return {};
  }

  size_t lastRow = 0;
  std::vector<std::string> FrontDecls;
  for (; cnt < childrenCnt;) {
    ts::Node node = TURoot.children[cnt];
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
      FrontDecls.emplace_back(std::move(textualContent));
      lastRow = node.endPoint().row;
      ++cnt;
    } else if (node.type() == ts::cpp::symbols::sym_comment.name) {
      size_t commentCnt = 0;
      bool realOrphan = true;
      auto [orphan, comment] = getComment(node, commentCnt, realOrphan);
      FrontDecls.emplace_back(std::move(comment));
      lastRow = node.endPoint().row;
      cnt += commentCnt;
    } else {
      break;
    }
  }

  return {FrontDecls, lastRow};
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
                                        size_t &commentCnt, bool &realOrphan) {
  assert(commentNode.type() == ts::cpp::symbols::sym_comment.name &&
         "invariant: node should be a comment");
  bool orphan = false;
  std::optional<ts::Node> prevNodeOpt = commentNode.prevSibling();
  if (prevNodeOpt.has_value()) {
    ts::Node prevNode = prevNodeOpt.value();
    if (prevNode.endPoint().row == commentNode.startPoint().row) {
      // 如果comment和上一个Node在同一行，那么我们特殊处理为Orphan Comment。
      // 因为我们对non-orphan
      // comment的判定为在上方注释该node的comment，否则会有comment消失的bug
      orphan = true;
      realOrphan = false;
    }
  }

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
  if (curComment.startPoint().row - prevComment.endPoint().row > 1) {
    return {true, ret.str()};
  } else {
    return {orphan, ret.str()};
  }
}

int getFollowingEOLs(const Node &node) {
  std::optional<ts::Node> nextSiblingOpt = node.nextSibling();

  while (nextSiblingOpt.has_value()) {
    // 如果下一个兄弟节点是;或{，则继续查找其下一个兄弟节点
    if (nextSiblingOpt.value().type() == ";" ||
        nextSiblingOpt.value().type() == "{") {
      nextSiblingOpt = nextSiblingOpt.value().nextSibling();
    } else {
      break;  // 不是;或{，则跳出循环
    }
  }

  if (!nextSiblingOpt.has_value()) {
    return 0;
  }

  int offset = nextSiblingOpt.value().startPoint().row - node.endPoint().row;
  return offset < 0 ? 0 : offset;
}

ClassInfo extractClassInfo(const std::string &code) {
  ClassInfo ret;

  const std::string pattern =
      R"(((template\s*<[^>]*>)?\s*(class|struct|union)\s*((\s*\[\[[^\]]+\]\])*)?\s*([a-zA-Z_][a-zA-Z0-9_:<>]*)?\s*(final)?\s*(:\s*[^\{]*)?\s*)\{)";
  std::string Final;
  re2::RE2 re(pattern);
  re2::StringPiece input(code);
  re2::StringPiece class_name_pieces;

  if (re2::RE2::PartialMatch(input, re, &ret.OriginalSignature,
                             &ret.TemplateParameterList, &ret.ClassKey,
                             &ret.Attrs, nullptr, &class_name_pieces, &Final,
                             &ret.BaseClause)) {
    ret.IsFinal = Final.size() != 0;
    ret.ClassName = class_name_pieces.ToString();
    if (ret.ClassName.size() && ret.ClassName.back() == ' ') {
      ret.ClassName.pop_back();
    }
    if (ret.Attrs.size() && ret.Attrs[0] == ' ') {
      ret.Attrs = ret.Attrs.substr(1);
    }

    // 计算行偏移
    ret.LineOffset =
        class_name_pieces.empty()
            ? 0
            : std::count(
                  input.begin(),
                  input.begin() + (class_name_pieces.data() - input.data()),
                  '\n');

    // 补充有qualified identifier的偏移计算
    size_t qualified_identifier_offset = 0;
    if (!class_name_pieces.empty()) {
      const re2::StringPiece::size_type angelOffset =
          class_name_pieces.find("<");
      if (angelOffset != re2::StringPiece::npos) {
        class_name_pieces = class_name_pieces.substr(0, angelOffset);
      }
      re2::StringPiece::size_type pos = class_name_pieces.rfind("::");
      if (pos != re2::StringPiece::npos) {
        qualified_identifier_offset = pos + 2;
      }
    }

    // 计算列偏移
    size_t last_newline =
        input.rfind('\n', class_name_pieces.data() - input.data());
    ret.ColOffset =
        class_name_pieces.empty()
            ? 0
            : (class_name_pieces.data() - input.data()) -
                  (last_newline == std::string::npos ? 0 : last_newline + 1) +
                  qualified_identifier_offset;
  } else {
    assert(false && "it seems that the code is not a class");
  }
  return ret;
}

// TODO(hwa): refactor, pass in the func name
FuncSpecialMemberInfo extractFuncSpecialMemberInfo(const std::string &code) {
  FuncSpecialMemberInfo ret;

  const std::string pattern =
      R"((template\s*<[^>]*>)?\s*((\s*\[\[[^\]]+\]\])*)?\s*((extern|static|constexpr|explicit|friend)\s*)*([~a-zA-Z_][a-zA-Z0-9_:]*)\s*\(([^)]*)\)\s*(:\s*[^{]*)?\s*(\{[^}]*\}|=\s*default|=\s*delete))";
  re2::RE2 re(pattern);
  re2::StringPiece input(code);
  re2::StringPiece func_name_pieces;
  re2::StringPiece body;
  std::string params, initList;

  if (re2::RE2::PartialMatch(input, re, &ret.TemplateParameterList, &ret.Attrs,
                             nullptr, &ret.BeforeFuncName, nullptr,
                             &func_name_pieces, &params, &initList, &body)) {
    ret.FuncName = func_name_pieces.ToString();
    if (ret.BeforeFuncName.size() && ret.BeforeFuncName.back() == ' ') {
      ret.BeforeFuncName.pop_back();
    }
    if (ret.Attrs.size() && ret.Attrs[0] == ' ') {
      ret.Attrs = ret.Attrs.substr(1);
    }
    assert(ret.FuncName.size() && "invariant: func name should not be empty");

    // Handle DefinitionType
    if (body.find("default") != std::string::npos) {
      ret.DefType = sa::FuncSpecialMemberNode::DefinitionType::Defaulted;
    } else if (body.find("delete") != std::string::npos) {
      ret.DefType = sa::FuncSpecialMemberNode::DefinitionType::Deleted;
    } else {
      ret.DefType = sa::FuncSpecialMemberNode::DefinitionType::Plain;
    }

    // Handle ParameterList
    std::string param;
    re2::StringPiece params_sp(params);
    while (RE2::Consume(&params_sp, R"(\s*([^,]+)\s*,?)", &param)) {
      ret.ParameterList.push_back(std::string(util::string_trim(param)));
    }

    // Handle InitList
    if (!initList.empty()) {
      std::string init;
      std::string InitList = initList.substr(1);
      re2::StringPiece initList_sp(InitList);
      while (RE2::Consume(&initList_sp, R"(\s*([^,]+)\s*,?)", &init)) {
        ret.InitList.push_back(std::string(util::string_trim(init)));
      }
    }

    // Handle OriginalSignature
    ret.OriginalSignature =
        ret.DefType == sa::FuncSpecialMemberNode::DefinitionType::Plain
            ? code.substr(0, code.find('{'))
            : code.substr(0, code.find('='));
    assert(ret.OriginalSignature.size() && "invariant: signature should exist");

    ret.LineOffset = std::count(
        input.begin(), input.begin() + (func_name_pieces.data() - input.data()),
        '\n');
    size_t qualified_identifier_offset = 0;
    if (!func_name_pieces.empty()) {
      re2::StringPiece::size_type pos = func_name_pieces.rfind("::");
      if (pos != re2::StringPiece::npos) {
        qualified_identifier_offset = pos + 2;
      }
    }
    size_t last_newline =
        input.rfind('\n', func_name_pieces.data() - input.data());
    ret.ColOffset = (func_name_pieces.data() - input.data()) -
                    (last_newline == std::string::npos ? 0 : last_newline + 1) +
                    qualified_identifier_offset;
  } else {
    assert(false && "it seems that the code is not a special member function");
  }

  return ret;
}

FuncOperatorCastInfo extractFuncOperatorCastInfo(const std::string &code) {
  FuncOperatorCastInfo ret;
  const std::string pattern =
      R"(((template\s*<[^>]*>)?\s*((\[\[[^\]]+\]\]\s*)*)?\s*(.*)(operator\s*[a-zA-Z_][a-zA-Z0-9_:]*)\s*\(([^)]*)\)([^{]*))\{)";
  re2::RE2 re(pattern);
  re2::StringPiece input(code);
  re2::StringPiece func_name_pieces;
  std::string paramListStr;

  if (re2::RE2::PartialMatch(input, re, &ret.OriginalSignature,
                             &ret.TemplateParameterList, &ret.Attrs, nullptr,
                             &ret.BeforeFuncName, &func_name_pieces,
                             &paramListStr, &ret.AfterParameterList)) {
    ret.FuncName = func_name_pieces.ToString();

    // Split ParameterList by comma and trim spaces
    std::istringstream iss(paramListStr);
    std::string param;
    while (std::getline(iss, param, ',')) {
      param.erase(std::remove_if(param.begin(), param.end(), ::isspace),
                  param.end());
      ret.ParameterList.push_back(param);
    }

    // Calculate LineOffset
    ret.LineOffset = std::count(
        input.begin(), input.begin() + (func_name_pieces.data() - input.data()),
        '\n');

    // Calculate ColOffset
    size_t last_newline =
        input.rfind('\n', func_name_pieces.data() - input.data());
    ret.ColOffset = (func_name_pieces.data() - input.data()) -
                    (last_newline == std::string::npos ? 0 : last_newline + 1);
  } else {
    assert(false && "it seems that the code is not an operator cast");
  }
  return ret;
}

FuncDefInfo extractFuncDefInfo(const std::string &code,
                               const std::string &FuncName) {
  FuncDefInfo ret;
  ret.FuncName = FuncName;

  using namespace std::string_literals;

  const std::string escapedFuncName = re2::RE2::QuoteMeta(FuncName);
  const std::string pattern =
      R"(((template\s*<[^>]*>)?\s*((\[\[[^\]]+\]\]\s*)*)?(.*)\s*)" + "("s +
      escapedFuncName + ")" + R"(\s*\(([^)]*)\)(\s*[^;{]*)))";

  re2::RE2 re(pattern);
  re2::StringPiece input(code);
  re2::StringPiece paramList;
  re2::StringPiece func_name_pieces;

  if (re2::RE2::PartialMatch(input, re, &ret.OriginalSignature,
                             &ret.TemplateParameterList, &ret.Attrs, nullptr,
                             &ret.BeforeFuncName, &func_name_pieces, &paramList,
                             &ret.AfterParameterList)) {
    // Split parameter list
    std::string paramStr = paramList.ToString();
    std::vector<std::string_view> splitted = util::string_split(paramStr, ",");
    for (auto &s : splitted) {
      ret.ParameterList.push_back(std::string(util::string_trim(s)));
    }

    ret.LineOffset = std::count(
        input.begin(), input.begin() + (func_name_pieces.data() - input.data()),
        '\n');
    size_t qualified_identifier_offset = 0;
    if (!func_name_pieces.empty()) {
      re2::StringPiece::size_type pos = func_name_pieces.rfind("::");
      if (pos != re2::StringPiece::npos) {
        qualified_identifier_offset = pos + 2;
      }
    }
    size_t last_newline =
        input.rfind('\n', func_name_pieces.data() - input.data());
    ret.ColOffset = (func_name_pieces.data() - input.data()) -
                    (last_newline == std::string::npos ? 0 : last_newline + 1) +
                    qualified_identifier_offset;
  } else {
    assert(false && "it seems that the code is not a function definition");
  }
  return ret;
}

}  // namespace ts
}  // namespace mergebot