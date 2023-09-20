//
// Created by whalien on 30/04/23.
//

#include "mergebot/core/semantic/pretty_printer.h"
#include "mergebot/core/model/node/EnumNode.h"
#include "mergebot/core/model/node/FuncDefNode.h"
#include "mergebot/core/model/node/FuncOperatorCastNode.h"
#include "mergebot/core/model/node/FuncSpecialMemberNode.h"
#include "mergebot/core/model/node/NamespaceNode.h"
#include "mergebot/core/model/node/TerminalNode.h"
#include "mergebot/core/model/node/TextualNode.h"
#include "mergebot/core/model/node/TranslationUnitNode.h"
#include "mergebot/core/model/node/TypeDeclNode.h"
#include "mergebot/utils/fileio.h"
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

std::string indentCodeLines(const std::string &str, int indent) { return str; }

std::string prettyPrintNode(const std::shared_ptr<SemanticNode> &Node) {
  std::stringstream ss;
  int indent = Node->StartPoint.value_or(ts::Point{0, 0}).column;
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
    if (TerminalNodePtr->Body.size() && llvm::isa<TextualNode>(Node.get())) {
      const char back = TerminalNodePtr->Body.back();
    }
    for (int i = 0; i < TerminalNodePtr->FollowingEOL; ++i) {
      ss << "\n";
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
      for (const auto &Child : Node->Children) {
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
  }
  return indentCodeLines(ss.str(), indent);
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
    ss << TURawPtr->HeaderGuard[2];
  }
  util::file_overwrite_content(DestFile, ss.str());
  return DestFile;
}
} // namespace mergebot::sa
