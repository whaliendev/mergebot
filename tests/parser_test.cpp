//
// Created by whalien on 10/05/23.
//

#include "mergebot/parser/parser.h"

#include <gtest/gtest.h>

#include "mergebot/filesystem.h"
#include "mergebot/parser/node.h"
#include "mergebot/parser/tree.h"
#include "mergebot/parser/utils.h"
#include "mergebot/utils/fileio.h"
#include "spdlog/spdlog.h"

namespace mergebot {
namespace sa {
TEST(ParserTest, FieldDeclTest) {
  // clang-format off
  std::string source = R"(
class A{
public:
 int a = 3;
 int b = 4;
};
)";
  // clang-format on
  ts::Parser parser(ts::cpp::language());
  std::shared_ptr<ts::Tree> tree = parser.parse(source);
  ts::Node rootNode = tree->rootNode();
  ts::Node ClassNode = rootNode.children[0];
  std::optional<ts::Node> BodyOfClass =
      ClassNode.getChildByFieldName(ts::cpp::fields::field_body.name);
  EXPECT_TRUE(BodyOfClass.has_value());
  std::vector<std::string> childrenSymbolNames;
  for (ts::Node const &Node : BodyOfClass.value().children) {
    childrenSymbolNames.push_back(
        ts::Symbol::nameOfSymbol(ts::cpp::language(), Node.symbol()));
  }
  ASSERT_TRUE(std::any_of(
      childrenSymbolNames.begin(), childrenSymbolNames.end(),
      [](const std::string &SymbolName) {
        return SymbolName == ts::cpp::symbols::sym_field_declaration.name;
      }));
}

TEST(Parser, GetTranslationUnitComment) {
  // clang-format off
  std::string expected = R"(
//===- DirectoryWatcher.h - Listens for directory file changes --*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
)";
  //clang-format on
  const std::string demoPath =
      (fs::current_path() / "mock" / "DirectoryWatcher.h").string();
  ASSERT_TRUE(fs::exists(demoPath));
  std::string fileSource = util::file_get_content(demoPath);
  ts::Parser parser(ts::cpp::language());
  std::shared_ptr<ts::Tree> tree = parser.parse(fileSource);
  ts::Node rootNode = tree->rootNode();
  ASSERT_EQ(rootNode.type(), "translation_unit");
  auto [commentCnt, comment] = ts::getTranslationUnitComment(rootNode);
  ASSERT_EQ(expected, "\n" + comment);
  ASSERT_EQ(7, commentCnt);
}

TEST(Parser, GetNodeComment) {
  // clang-format off
  std::string expected = R"(
/// Provides notifications for file changes in a directory.
///
/// Invokes client-provided function on every filesystem event in the watched
/// directory. Initially the watched directory is scanned and for every file
/// found, an event is synthesized as if the file was added.
///
/// This is not a general purpose directory monitoring tool - list of
/// limitations follows.
///
/// Only flat directories with no subdirectories are supported. In case
/// subdirectories are present the behavior is unspecified - events *might* be
/// passed to Receiver on macOS (due to FSEvents being used) while they
/// *probably* won't be passed on Linux (due to inotify being used).
///
/// Known potential inconsistencies
/// - For files that are deleted befor the initial scan processed them, clients
/// might receive Removed notification without any prior Added notification.
/// - Multiple notifications might be produced when a file is added to the
/// watched directory during the initial scan. We are choosing the lesser evil
/// here as the only known alternative strategy would be to invalidate the
/// watcher instance and force user to create a new one whenever filesystem
/// event occurs during the initial scan but that would introduce continuous
/// restarting failure mode (watched directory is not always "owned" by the same
/// process that is consuming it). Since existing clients can handle duplicate
/// events well, we decided for simplicity.
///
/// Notifications are provided only for changes done through local user-space
/// filesystem interface. Specifically, it's unspecified if notification would
/// be provided in case of a:
/// - a file mmap-ed and changed
/// - a file changed via remote (NFS) or virtual (/proc) FS access to monitored
/// directory
/// - another filesystem mounted to the watched directory
///
/// No support for LLVM VFS.
///
/// It is unspecified whether notifications for files being deleted are sent in
/// case the whole watched directory is sent.
///
/// Directories containing "too many" files and/or receiving events "too
/// frequently" are not supported - if the initial scan can't be finished before
/// the watcher instance gets invalidated (see WatcherGotInvalidated) there's no
/// good error handling strategy - the only option for client is to destroy the
/// watcher, restart watching with new instance and hope it won't repeat.
)";
  // clang-format on
  const std::string demoPath =
      (fs::current_path() / "mock" / "DirectoryWatcher.h").string();
  ASSERT_TRUE(fs::exists(demoPath));
  std::string fileSource = util::file_get_content(demoPath);
  ts::Parser parser(ts::cpp::language());
  std::shared_ptr<ts::Tree> tree = parser.parse(fileSource);
  ts::Node rootNode = tree->rootNode();
  ts::Node namespaceDefi =
      rootNode.children[7].children[2 + 7];  // 2 is ifdef/ifndef and identifier
  ts::Node preprocIfdef = rootNode.children[7];
  ASSERT_EQ(preprocIfdef.type(), "preproc_ifdef");
  spdlog::info("is named: {}, text: {}", preprocIfdef.children[0].isNamed(),
               preprocIfdef.children[0].text());
  spdlog::info("is named: {}, text: {}", preprocIfdef.children[1].isNamed(),
               preprocIfdef.children[1].text());
  ASSERT_EQ(namespaceDefi.type(), "namespace_definition");
  ts::Node namespaceBody =
      namespaceDefi.getChildByFieldName(ts::cpp::fields::field_body.name)
          .value();
  ts::Node classDirectoryWalker = namespaceBody.namedChildren()[44];
  std::string comment = ts::getNodeComment(classDirectoryWalker);
  ASSERT_EQ(expected, "\n" + comment);
}

TEST(Parser, Type) {
  // clang-format off
  std::string source = R"(
/// comment
/// comment

#ifndef AAA
#define AAA
namespace mergebot {
namespace sa {
/// class A represents balabala
/// balala
/// babala
/// lalaba
class A {
public:
  A(const B& b): b(b) {}
private:
  B b;
};
} // namespace sa
} // namespace mergebot

#endif // comment
)";
  // clang-format on

  ts::Parser parser(ts::cpp::language());
  std::shared_ptr<ts::Tree> tree = parser.parse(source);
  ts::Node Node = tree->rootNode().children[2].namedChildren()[2];
  spdlog::info("node text: {}", Node.text());
  spdlog::info("node type: {}", Node.type());
  spdlog::info("node is named: {}", Node.isNamed());
}
}  // namespace sa
}  // namespace mergebot