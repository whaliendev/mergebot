//
// Created by whalien on 30/04/23.
//

#include "mergebot/core/semantic/pretty_printer.h"
#include "mergebot/core/model/enum/AccessSpecifierKind.h"
#include "mergebot/core/model/enum/ConflictMark.h"
#include "mergebot/core/model/node/EnumNode.h"
#include "mergebot/core/model/node/FuncDefNode.h"
#include "mergebot/core/model/node/FuncOperatorCastNode.h"
#include "mergebot/core/model/node/FuncSpecialMemberNode.h"
#include "mergebot/core/model/node/NamespaceNode.h"
#include "mergebot/core/model/node/TerminalNode.h"
#include "mergebot/core/model/node/TranslationUnitNode.h"
#include "mergebot/core/model/node/TypeDeclNode.h"
#include "mergebot/utils/fileio.h"
#include <magic_enum.hpp>
namespace mergebot::sa {
namespace details {
std::string printFuncDefNodeSignature(const FuncDefNode *func) {
  std::stringstream ss;
  std::string TemplateParameterList = func->TemplateParameterList;
  if (TemplateParameterList.size() >= 10) {
    ss << TemplateParameterList << "\n";
  } else {
    ss << (TemplateParameterList.size() ? TemplateParameterList + " " : "");
  }
  ss << func->Attrs << func->BeforeFuncName << func->DisplayName;
  ss << fmt::format("({})", fmt::join(func->ParameterList.begin(),
                                      func->ParameterList.end(), ", "));
  ss << func->AfterParameterList;
  return ss.str();
}

std::string printFuncOperatorCastSignature(const FuncOperatorCastNode *func) {
  std::stringstream ss;
  std::string TemplateParameterList = func->TemplateParameterList;
  if (TemplateParameterList.size() >= 10) {
    ss << TemplateParameterList << "\n";
  } else {
    ss << (TemplateParameterList.size() ? TemplateParameterList + " " : "");
  }
  ss << func->Attrs << func->BeforeFuncName << func->DisplayName;
  ss << fmt::format("({})", fmt::join(func->ParameterList.begin(),
                                      func->ParameterList.end(), ", "));
  ss << func->AfterParameterList;
  return ss.str();
}

std::string printFuncSpecialMemberSignature(const FuncSpecialMemberNode *func) {
  std::stringstream ss;
  std::string TemplateParameterList = func->TemplateParameterList;
  if (TemplateParameterList.size() >= 10) {
    ss << TemplateParameterList << "\n";
  } else {
    ss << (TemplateParameterList.size() ? TemplateParameterList + " " : "");
  }
  ss << func->Attrs << func->BeforeFuncName << func->DisplayName;
  ss << fmt::format("({})", fmt::join(func->ParameterList.begin(),
                                      func->ParameterList.end(), ", "));
  if (func->InitList.size()) {
    ss << fmt::format(
        ": {}", fmt::join(func->InitList.begin(), func->InitList.end(), ", "));
    ss << " ";
  }
  if (func->DefType != FuncSpecialMemberNode::Plain) {
    ss << ((func->DefType == FuncSpecialMemberNode::Defaulted) ? " = default;"
                                                               : " = delete;");
  }
  return ss.str();
}

std::string indentCodeLines(const std::string &str, int indent,
                            int collapseSpaceCnt) {
  std::istringstream input_stream(str);
  std::string line;
  std::vector<std::string> indentedLines;
  bool addLastNewLine = str.size() > 0 && str.back() == '\n';

  while (std::getline(input_stream, line)) {
    std::size_t firstCharPos = line.find_first_not_of(" \t");
    if (firstCharPos == std::string::npos) {
      firstCharPos = line.length();
    }

    if (line.find(magic_enum::enum_name(ConflictMark::OURS)) !=
            std::string::npos ||
        line.find(magic_enum::enum_name(ConflictMark::BASE)) !=
            std::string::npos ||
        line.find(magic_enum::enum_name(ConflictMark::THEIRS)) !=
            std::string::npos ||
        line.find(magic_enum::enum_name(ConflictMark::END)) !=
            std::string::npos) {
      indentedLines.push_back(line);
      continue;
    }

    if (firstCharPos < static_cast<size_t>(indent)) {
      int spaceCnt = indent - firstCharPos - collapseSpaceCnt;
      if (spaceCnt < 0) {
        spaceCnt = 0;
      }
      indentedLines.push_back(std::string(spaceCnt, ' ') + line);
    } else {
      indentedLines.push_back(line);
    }
  }

  // Use stringstream for efficient concatenation
  std::stringstream result_stream;
  for (const auto &indentedLine : indentedLines) {
    result_stream << indentedLine << '\n';
  }

  std::string indentedStr = result_stream.str();
  if (indentedStr.size() && !addLastNewLine) {
    indentedStr.pop_back();
  }
  return indentedStr;
}

std::string prettyPrintNode(const std::shared_ptr<SemanticNode> &Node) {
  std::stringstream ss;
  int indent = Node->StartPoint.value_or(ts::Point{0, 0}).column;
  int collapseSpaceCnt = 0;
  std::string nodeStr;

  if (llvm::isa<TerminalNode>(Node.get())) {
    auto TerminalNodePtr = llvm::cast<TerminalNode>(Node.get());
    ss << Node->Comment;
    if (llvm::isa<FuncDefNode>(Node.get()) ||
        llvm::isa<FuncOperatorCastNode>(Node.get()) ||
        llvm::isa<FuncSpecialMemberNode>(Node.get())) {
      if (TerminalNodePtr->SigUnchanged) {
        ss << Node->OriginalSignature;
      } else {
        if (auto FuncDef = llvm::dyn_cast<FuncDefNode>(Node.get())) {
          ss << printFuncDefNodeSignature(FuncDef);
        } else if (auto FuncCast =
                       llvm::dyn_cast<FuncOperatorCastNode>(Node.get())) {
          ss << printFuncOperatorCastSignature(FuncCast);
        } else if (auto FuncSpecial =
                       llvm::dyn_cast<FuncSpecialMemberNode>(Node.get())) {
          ss << printFuncSpecialMemberSignature(FuncSpecial);
        }
      }
    }
    ss << TerminalNodePtr->Body;
    for (int i = 0; i < TerminalNodePtr->FollowingEOL; ++i) {
      ss << "\n";
    }
    nodeStr = ss.str();
    if (nodeStr.size() && nodeStr.back() != '\n') {
      collapseSpaceCnt = nodeStr.size();
    }
  } else {
    // composite node
    assert(llvm::isa<CompositeNode>(Node.get()));
    auto CompositePtr = llvm::cast<CompositeNode>(Node.get());
    if (!llvm::isa<TranslationUnitNode>(Node.get())) {
      ss << Node->Comment;
      ss << Node->OriginalSignature << "{";
      for (int i = 0; i < CompositePtr->BeforeFirstChildEOL; ++i) {
        ss << "\n";
      }

      AccessSpecifierKind PrevAccessSpecifier = AccessSpecifierKind::None;
      for (const auto &Child : Node->Children) {
        if (Child->AccessSpecifier != PrevAccessSpecifier &&
            Child->AccessSpecifier != AccessSpecifierKind::None &&
            Child->AccessSpecifier != AccessSpecifierKind::Default) {
          ss << magic_enum::enum_name(Child->AccessSpecifier) << "\n";
          PrevAccessSpecifier = Child->AccessSpecifier;
        }
        ss << prettyPrintNode(Child);
      }
      ss << "}";

      if (auto NamespacePtr = llvm::dyn_cast<NamespaceNode>(Node.get())) {
        ss << " " + NamespacePtr->NSComment;
      } else if (llvm::isa<TypeDeclNode>(Node.get()) ||
                 llvm::isa<EnumNode>(Node.get())) {
        ss << ";";
      }

      for (int i = 0; i < Node->FollowingEOL; ++i) {
        ss << "\n";
      }
      ss << "\n\n";
    } else {
      for (int i = 0; i < CompositePtr->BeforeFirstChildEOL; ++i) {
        ss << "\n";
      }

      for (const auto &Child : Node->Children) {
        ss << prettyPrintNode(Child);
      }
    }
    nodeStr = ss.str();
  }
  return indentCodeLines(nodeStr, indent, collapseSpaceCnt);
}
} // namespace details

std::string PrettyPrintTU(const std::shared_ptr<SemanticNode> &TUNode,
                          const std::string &DestDir) {
  assert(llvm::isa<TranslationUnitNode>(TUNode.get()));
  TranslationUnitNode *TURawPtr = llvm::cast<TranslationUnitNode>(TUNode.get());
  std::string DestFile = (fs::path(DestDir) / TURawPtr->DisplayName).string();
  fs::create_directories(fs::path(DestFile).parent_path());

  std::stringstream ss;

  ss << TURawPtr->Comment << "\n";

  if (TURawPtr->IsHeader) {
    if (TURawPtr->TraditionGuard) {
      assert(TURawPtr->HeaderGuard.size() >= 3);
      ss << TURawPtr->HeaderGuard[0] << "\n"
         << TURawPtr->HeaderGuard[1] << "\n";
    } else {
      ss << TURawPtr->HeaderGuard[0]; // pragma once contains an EOL
    }
  }

  for (const auto &FrontDecl : TURawPtr->FrontDecls) {
    ss << FrontDecl << "\n";
  }

  ss << details::prettyPrintNode(TUNode);

  if (TURawPtr->IsHeader && TURawPtr->TraditionGuard) {
    assert(TURawPtr->HeaderGuard.size() >= 3);
    ss << TURawPtr->HeaderGuard[2];
  }
  util::file_overwrite_content(DestFile, ss.str());
  return DestFile;
}
} // namespace mergebot::sa
