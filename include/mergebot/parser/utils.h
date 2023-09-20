//
// Created by whalien on 04/09/23.
//

#ifndef MB_INCLUDE_MERGEBOT_PARSER_PARSER_UTILITY_H
#define MB_INCLUDE_MERGEBOT_PARSER_PARSER_UTILITY_H

#include <fmt/format.h>

#include <string>
#include <utility>

#include "mergebot/core/model/node/FuncSpecialMemberNode.h"
#include "mergebot/parser/node.h"

namespace mergebot {
namespace ts {

std::pair<size_t, std::string> getTranslationUnitComment(const ts::Node &root);

std::string getNodeComment(const ts::Node &node);

int beforeFirstChildEOLs(const ts::Node &node);

int nextSiblingDistance(const ts::Node &node, bool named = false);

/// \brief get the header guard of a header file
/// \param TUNode translation unit node
/// \param HeaderGuardStart child index to start match
/// \return std::pair of bool and header guard string, bool indicates whether
/// the header guard is a tradition guard
std::pair<bool, std::vector<std::string>> getHeaderGuard(
    const ts::Node &TUNode, size_t &BeforeBodyChildCnt, ts::Node &TURoot);

std::pair<std::vector<std::string>, size_t> getFrontDecls(
    const ts::Node &TURoot, size_t &cnt);

std::pair<bool, std::string> getComment(const ts::Node &commentNode,
                                        size_t &commentCnt);

bool isTypeDecl(const ts::Node &node);

int getFollowingEOLs(const ts::Node &node);

struct ClassInfo {
  std::string OriginalSignature;
  std::string TemplateParameterList;
  std::string ClassKey;
  std::string Attrs;
  std::string ClassName;
  bool IsFinal;
  std::string BaseClause;
  int LineOffset;
  int ColOffset;

  bool operator==(const ClassInfo &other) const {
    return OriginalSignature == other.OriginalSignature &&
           TemplateParameterList == other.TemplateParameterList &&
           ClassKey == other.ClassKey && Attrs == other.Attrs &&
           ClassName == other.ClassName && IsFinal == other.IsFinal &&
           BaseClause == other.BaseClause && LineOffset == other.LineOffset &&
           ColOffset == other.ColOffset;
  }

  friend std::ostream &operator<<(std::ostream &os, const ts::ClassInfo &obj) {
    std::string formatted_output = fmt::format(
        "ClassInfo(\n"
        "  OriginalSignature={},\n"
        "  TemplateParameterList={},\n"
        "  ClassKey={},\n"
        "  Attrs={},\n"
        "  ClassName={},\n"
        "  IsFinal={},\n"
        "  BaseClause={},\n"
        "  LineOffset={},\n"
        "  ColOffset={}\n"
        ")",
        obj.OriginalSignature, obj.TemplateParameterList, obj.ClassKey,
        obj.Attrs, obj.ClassName, obj.IsFinal, obj.BaseClause, obj.LineOffset,
        obj.ColOffset);
    os << formatted_output;
    return os;
  }
};

ClassInfo extractClassInfo(const std::string &code);

struct FuncSpecialMemberInfo {
  sa::FuncSpecialMemberNode::DefinitionType DefType;
  std::string OriginalSignature;
  std::string TemplateParameterList;
  std::string Attrs;
  std::string BeforeFuncName;
  std::string FuncName;
  std::vector<std::string> ParameterList;
  std::vector<std::string> InitList;
  int LineOffset;
  int ColOffset;

  bool operator==(const FuncSpecialMemberInfo &other) const {
    return OriginalSignature == other.OriginalSignature &&
           TemplateParameterList == other.TemplateParameterList &&
           Attrs == other.Attrs && BeforeFuncName == other.BeforeFuncName &&
           FuncName == other.FuncName && ParameterList == other.ParameterList &&
           InitList == other.InitList && LineOffset == other.LineOffset &&
           ColOffset == other.ColOffset;
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const ts::FuncSpecialMemberInfo &obj) {
    std::string formatted_output = fmt::format(
        "FuncSpecialMemberInfo(\n"
        "  OriginalSignature={},\n"
        "  TemplateParameterList={},\n"
        "  Attrs={},\n"
        "  BeforeFuncName={},\n"
        "  FuncName={},\n"
        "  ParameterList={},\n"
        "  InitList={},\n"
        "  LineOffset={},\n"
        "  ColOffset={}\n"
        ")",
        obj.OriginalSignature, obj.TemplateParameterList, obj.Attrs,
        obj.BeforeFuncName, obj.FuncName, fmt::join(obj.ParameterList, ", "),
        fmt::join(obj.InitList, ", "), obj.LineOffset, obj.ColOffset);
    return os << formatted_output;
  }
};

FuncSpecialMemberInfo extractFuncSpecialMemberInfo(const std::string &code);

struct FuncOperatorCastInfo {
  std::string OriginalSignature;
  std::string TemplateParameterList;
  std::string Attrs;
  std::string BeforeFuncName;
  std::string FuncName;
  std::vector<std::string> ParameterList;
  std::string AfterParameterList;
  int LineOffset;
  int ColOffset;

  bool operator==(const FuncOperatorCastInfo &other) const {
    return OriginalSignature == other.OriginalSignature &&
           TemplateParameterList == other.TemplateParameterList &&
           Attrs == other.Attrs && BeforeFuncName == other.BeforeFuncName &&
           FuncName == other.FuncName && ParameterList == other.ParameterList &&
           AfterParameterList == other.AfterParameterList &&
           LineOffset == other.LineOffset && ColOffset == other.ColOffset;
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const ts::FuncOperatorCastInfo &obj) {
    std::string formatted_output = fmt::format(
        "FuncOperatorCastInfo(\n"
        "  OriginalSignature={},\n"
        "  TemplateParameterList={},\n"
        "  Attrs={},\n"
        "  BeforeFuncName={},\n"
        "  FuncName={},\n"
        "  ParameterList={},\n"
        "  AfterParameterList={},\n"
        "  LineOffset={},\n"
        "  ColOffset={}\n"
        ")",
        obj.OriginalSignature, obj.TemplateParameterList, obj.Attrs,
        obj.BeforeFuncName, obj.FuncName, fmt::join(obj.ParameterList, ", "),
        obj.AfterParameterList, obj.LineOffset, obj.ColOffset);
    return os << formatted_output;
  }
};

FuncOperatorCastInfo extractFuncOperatorCastInfo(const std::string &code);

// 手动解析，正则不可靠
struct FuncDefInfo {
  std::string OriginalSignature;
  std::string TemplateParameterList;
  std::string Attrs;
  std::string BeforeFuncName;
  std::string FuncName;
  std::vector<std::string> ParameterList;
  std::string AfterParameterList;
  int LineOffset;
  int ColOffset;

  bool operator==(const FuncDefInfo &other) const {
    return OriginalSignature == other.OriginalSignature &&
           TemplateParameterList == other.TemplateParameterList &&
           Attrs == other.Attrs && BeforeFuncName == other.BeforeFuncName &&
           FuncName == other.FuncName && ParameterList == other.ParameterList &&
           AfterParameterList == other.AfterParameterList &&
           LineOffset == other.LineOffset && ColOffset == other.ColOffset;
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const ts::FuncDefInfo &obj) {
    std::string formatted_output = fmt::format(
        "FuncDefInfo(\n"
        "  OriginalSignature={},\n"
        "  TemplateParameterList={},\n"
        "  Attrs={},\n"
        "  BeforeFuncName={},\n"
        "  FuncName={},\n"
        "  ParameterList={},\n"
        "  AfterParameterList={},\n"
        "  LineOffset={},\n"
        "  ColOffset={}\n"
        ")",
        obj.OriginalSignature, obj.TemplateParameterList, obj.Attrs,
        obj.BeforeFuncName, obj.FuncName, fmt::join(obj.ParameterList, ", "),
        obj.AfterParameterList, obj.LineOffset, obj.ColOffset);
    return os << formatted_output;
  }
};

FuncDefInfo extractFuncDefInfo(const std::string &code,
                               const std::string &FuncName);

}  // namespace ts
}  // namespace mergebot

#endif  // MB_INCLUDE_MERGEBOT_PARSER_PARSER_UTILITY_H
