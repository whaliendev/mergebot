//
// Created by whalien on 13/03/23.
//

#include "mergebot/core/handler/TextBasedHandler.h"
#include "mergebot/core/model/ConflictBlock.h"
#include "mergebot/core/model/ConflictFile.h"
#include "mergebot/core/model/enum/ConflictMark.h"
#include "mergebot/core/model/enum/Side.h"
#include "mergebot/core/sa_utility.h"
#include "mergebot/filesystem.h"
// #include "mergebot/parser/languages/cpp.h"
#include "mergebot/parser/node.h"
#include "mergebot/parser/parser.h"
#include "mergebot/parser/symbol.h"
#include "mergebot/parser/tree.h"
#include "mergebot/server/vo/ResolutionResultVO.h"
#include "mergebot/utils/fileio.h"
#include "mergebot/utils/stringop.h"
#include <algorithm>
#include <cctype>
#include <llvm/Support/ErrorOr.h>
#include <llvm/Support/MemoryBuffer.h>
#include <magic_enum.hpp>
#include <memory>
#include <queue>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace mergebot {
namespace sa {

namespace _details {
bool isConflictBlockInBuffer(std::string const &Source, char const *Start,
                             char const *End, size_t Size) {
  if (Size < Source.length()) {
    return false;
  }
  const char *FoundPos = std::search(Start, End, Source.begin(), Source.end());
  return FoundPos != End;
}

std::string extractConflictBlock(int Index, const char *Start, size_t Size) {
  std::string BeginPattern = "<<<<<<<";
  std::string EndPattern = ">>>>>>>";
  auto Begin = Start;
  auto End = Start + Size;

  int Count = 0;
  auto It = Begin;
  while ((It = std::search(It, End, BeginPattern.begin(),
                           BeginPattern.end())) != End) {
    auto BlockBegin = It + BeginPattern.size();
    auto BlockEnd =
        std::search(BlockBegin, End, EndPattern.begin(), EndPattern.end());
    if (++Count == Index) {
      return std::string(It, BlockEnd + EndPattern.size());
    }
    It = BlockEnd + EndPattern.size();
  }
  return "";
}

bool isCppSource(std::string const &Filename) {
  const char *CppSourceExt[] = {".cc",  ".cpp", ".cxx", ".cx",
                                ".c++", ".C",   ".c++", ".c"};
  for (auto it = std::begin(CppSourceExt); it != std::end(CppSourceExt); ++it) {
    if (util::ends_with(Filename, *it)) {
      return true;
    }
  }
  return false;
}

bool isCppHeader(std::string const &Filename) {
  const char *CppHeaderExt[] = {".hpp", ".h"};
  for (auto it = std::begin(CppHeaderExt); it != std::end(CppHeaderExt); ++it) {
    if (util::ends_with(Filename, *it)) {
      return true;
    }
  }
  return false;
}

bool isSubSequence(std::vector<std::string_view> const &Seq1,
                   std::vector<std::string_view> const &Seq2) {
  size_t i = 0;
  size_t j = 0;
  while (i < Seq1.size() && j < Seq2.size()) {
    if (Seq1[i] == Seq2[j]) {
      j++;
    }
    i++;
  }
  return j == Seq2.size();
}

bool parseDeclarations(std::string const &Code,
                       std::vector<std::string> &Decls) {
  if (Code.empty())
    return false;
  ts::Parser Parser(ts::cpp::language());

  std::shared_ptr<ts::Tree> Tree = Parser.parse(std::string(Code));
  ts::Node RootNode = Tree->rootNode();
  for (ts::Node const &Node : RootNode.namedChildren()) {
    std::string SymbolName =
        ts::Symbol::nameOfSymbol(ts::cpp::language(), Node.symbol());
    if (SymbolName != ts::cpp::symbols::sym_comment.name &&
        SymbolName.find("declaration") == std::string::npos) {
      return false;
    }
    if (SymbolName == ts::cpp::symbols::sym_comment.name) {
      continue;
    }

    // declaration
    std::string Decl = Node.text();
    std::optional<ts::Node> PrevSibling = Node.prevSibling();
    if (PrevSibling.has_value() &&
        ts::Symbol::nameOfSymbol(ts::cpp::language(),
                                 PrevSibling.value().symbol()) ==
            ts::cpp::symbols::sym_comment.name) {
      std::string comment = PrevSibling.value().text();
      Decl = comment + "\n" + Node.text();
      PrevSibling = PrevSibling.value().prevSibling();
    }
    Decls.push_back(std::move(Decl));
  }
  return true;
}

bool parseDefinitions(std::string const &Code,
                      std::vector<std::string> &Definitions) {
  if (Code.empty())
    return false;
  ts::Parser Parser(ts::cpp::language());

  std::shared_ptr<ts::Tree> Tree = Parser.parse(std::string(Code));
  ts::Node RootNode = Tree->rootNode();
  bool EverDefinition = false;
  for (ts::Node const &Node : RootNode.namedChildren()) {
    std::string SymbolName =
        ts::Symbol::nameOfSymbol(ts::cpp::language(), Node.symbol());
    if (SymbolName != ts::cpp::symbols::sym_comment.name &&
        SymbolName.find("definition") == std::string::npos &&
        SymbolName.find("declaration") == std::string::npos) {
      return false;
    }
    if (SymbolName == ts::cpp::symbols::sym_comment.name) {
      continue;
    }

    // declaration or definition
    std::string Definition = Node.text();
    if (ts::Symbol::nameOfSymbol(ts::cpp::language(), Node.symbol())
            .find("definition") != std::string::npos) {
      EverDefinition = true;
    }
    std::optional<ts::Node> PrevSibling = Node.prevSibling();
    while (PrevSibling.has_value() &&
           ts::Symbol::nameOfSymbol(ts::cpp::language(),
                                    PrevSibling.value().symbol()) ==
               ts::cpp::symbols::sym_comment.name) {
      std::string comment = PrevSibling.value().text();
      Definition = comment + "\n" + Node.text();
      PrevSibling = PrevSibling.value().prevSibling();
    }
    Definitions.push_back(std::move(Definition));
  }
  return EverDefinition;
}

bool parseInclusions(std::string const &Code,
                     std::vector<std::string> &Inclusions) {
  if (Code.empty())
    return false;
  ts::Parser Parser(ts::cpp::language());

  std::shared_ptr<ts::Tree> Tree = Parser.parse(std::string(Code));
  ts::Node Root = Tree->rootNode();
  for (ts::Node const &Node : Root.namedChildren()) {
    std::string SymbolName =
        ts::Symbol::nameOfSymbol(ts::cpp::language(), Node.symbol());
    // only parse #include
    if (SymbolName != ts::cpp::symbols::sym_preproc_include.name) {
      return false;
    }
    Inclusions.push_back(Node.text());
  }
  return true;
}

bool topologicalSort(
    const std::unordered_map<std::string, std::unordered_set<std::string>>
        &graph,
    std::vector<std::string> &result) {
  std::unordered_map<std::string, int> inDegree;
  for (const auto &p : graph) {
    inDegree[p.first] = 0;
  }

  // Calculate in-degree for each node
  for (const auto &p : graph) {
    for (const auto &child : p.second) {
      inDegree[child]++;
    }
  }

  // Find nodes with in-degree 0 as the starting point
  std::queue<std::string> queue;
  for (const auto &p : inDegree) {
    if (p.second == 0) {
      queue.push(p.first);
    }
  }

  while (!queue.empty()) {
    std::string node = queue.front();
    queue.pop();
    result.push_back(node);
    for (const auto &child : graph.at(node)) {
      inDegree[child]--;
      if (inDegree[child] == 0) {
        queue.push(child);
      }
    }
  }

  // Check if there is a cycle
  return result.size() == graph.size();
}

bool mergeVectors(std::vector<std::string> const &Vec1,
                  std::vector<std::string> const &Vec2,
                  std::vector<std::string> &result) {
  if (Vec1.empty() && Vec2.empty()) {
    result.clear();
    return true;
  }

  // First, deduplicate the input vectors while preserving order
  std::vector<std::string> uniqueVec1, uniqueVec2;
  {
    std::unordered_set<std::string> seen;
    for (const auto &item : Vec1) {
      if (seen.insert(item).second) {
        uniqueVec1.push_back(item);
      }
    }
    seen.clear();
    for (const auto &item : Vec2) {
      if (seen.insert(item).second) {
        uniqueVec2.push_back(item);
      }
    }
  }

  util::DependencyGraph graph;

  // Add all nodes from both vectors
  for (const auto &line : uniqueVec1) {
    graph.addNode(line);
  }
  for (const auto &line : uniqueVec2) {
    graph.addNode(line);
  }

  // Add dependencies from Vec1
  if (!uniqueVec1.empty()) {
    for (size_t i = 0; i < uniqueVec1.size() - 1; ++i) {
      if (!graph.addEdge(uniqueVec1[i], uniqueVec1[i + 1])) {
        spdlog::error("Failed to add edge between {} and {}", uniqueVec1[i],
                      uniqueVec1[i + 1]);
        return false;
      }
    }
  }

  // Add dependencies from Vec2
  if (!uniqueVec2.empty()) {
    for (size_t i = 0; i < uniqueVec2.size() - 1; ++i) {
      if (!graph.addEdge(uniqueVec2[i], uniqueVec2[i + 1])) {
        spdlog::error("Failed to add edge between {} and {}", uniqueVec2[i],
                      uniqueVec2[i + 1]);
        return false;
      }
    }
  }

  // Perform topological sort
  bool success = graph.topologicalSort(result);
  if (!success) {
    spdlog::error("Failed to perform topological sort - cycle detected");
    result.clear();
  }

  return success;
}
} // namespace _details

bool TextBasedHandler::checkOneSideDelta(std::string_view Our,
                                         std::string_view Their,
                                         ConflictFile const &CF,
                                         server::BlockResolutionResult &BRR) {
  if (Our.empty() || Their.empty())
    return false;
  using namespace llvm;
  fs::path Relative = fs::relative(CF.Filename, Meta.ProjectPath);
  fs::path ConflictFile = fs::path(Meta.MSCacheDir) / "conflicts" / Relative;
  llvm::ErrorOr<std::unique_ptr<MemoryBuffer>> FileOrErr =
      MemoryBuffer::getFile(ConflictFile.string(), false, false);
  if (auto Err = FileOrErr.getError()) {
    spdlog::info("fail to read conflict file [{}], err message: {}",
                 ConflictFile.string(), Err.message());
    return false;
  }

  std::unique_ptr<MemoryBuffer> File = std::move(FileOrErr.get());
  std::string CodeRange = _details::extractConflictBlock(
      BRR.index, File->getBufferStart(), File->getBufferSize());
  if (CodeRange.empty())
    return false;
  std::string_view OurCode = extractCodeFromConflictRange(
      CodeRange, magic_enum::enum_name(ConflictMark::OURS),
      magic_enum::enum_name(ConflictMark::BASE));
  std::string_view BaseCode = extractCodeFromConflictRange(
      CodeRange, magic_enum::enum_name(ConflictMark::BASE),
      magic_enum::enum_name(ConflictMark::THEIRS));
  std::string_view TheirCode = extractCodeFromConflictRange(
      CodeRange, magic_enum::enum_name(ConflictMark::THEIRS),
      magic_enum::enum_name(ConflictMark::END));
  std::string OurDeflated =
      util::removeCommentsAndSpaces(util::doMacroExpansion(OurCode));
  std::string BaseDeflated =
      util::removeCommentsAndSpaces(util::doMacroExpansion(BaseCode));
  std::string TheirDeflated =
      util::removeCommentsAndSpaces(util::doMacroExpansion(TheirCode));
  if (OurDeflated == BaseDeflated) {
    BRR.code = std::string(Their);
    BRR.desc = "De facto one-sided modification, accept their side.";
    spdlog::info(
        "one side delta detected for conflict block [{}] in conflict file [{}]",
        BRR.index, CF.Filename);
    return true;
  }
  if (TheirDeflated == BaseDeflated) {
    BRR.code = std::string(Our);
    BRR.desc = "De facto one-sided modification, accept their side.";
    spdlog::info(
        "one side delta detected for conflict block [{}] in conflict file [{}]",
        BRR.index, CF.Filename);
    return true;
  }
  return false;
}

bool TextBasedHandler::checkInclusion(std::string_view Our,
                                      std::string_view Their,
                                      ConflictFile const &CF,
                                      server::BlockResolutionResult &BRR) {
  std::string DeflatedOurs = util::removeCommentsAndSpaces(std::string(Our));
  std::string DeflatedTheirs =
      util::removeCommentsAndSpaces(std::string(Their));

  bool OurContain = DeflatedOurs.size() > DeflatedTheirs.size() &&
                    DeflatedOurs.find(DeflatedTheirs) != std::string::npos;
  bool TheirContain = DeflatedTheirs.size() > DeflatedOurs.size() &&
                      DeflatedTheirs.find(DeflatedOurs) != std::string::npos;
  if (!OurContain && !TheirContain) {
    return false;
  }

  if (_details::isCppSource(CF.Filename)) {
    std::vector<std::string_view> OurLineVec = util::string_split(Our, "\n");
    std::vector<std::string_view> TheirLineVec =
        util::string_split(Their, "\n");
    if (OurLineVec.size() > TheirLineVec.size()) {
      if (_details::isSubSequence(OurLineVec, TheirLineVec)) {
        BRR.code = std::string(Our);
        BRR.desc = "Added code or method extraction, accept our side.";
        spdlog::info("more check or method extraction found for conflict "
                     "block[{}] in conflict file[{}]",
                     BRR.index, CF.Filename);
        return true;
      }
    } else {
      if (_details::isSubSequence(TheirLineVec, OurLineVec)) {
        BRR.code = std::string(Their);
        BRR.desc = "Added code or method extraction, accept our side";
        spdlog::info("add check or method extraction found for conflict "
                     "block[{}] in conflict file[{}]",
                     BRR.index, CF.Filename);
        return true;
      }
    }
  }

  if (_details::isCppHeader(CF.Filename)) {
    ts::Parser Parser(ts::cpp::language());

    std::shared_ptr<ts::Tree> OurTree = Parser.parse(std::string(Our));
    ts::Node OurRootNode = OurTree->rootNode();
    for (ts::Node const &Node : OurRootNode.namedChildren()) {
      std::string SymbolName =
          ts::Symbol::nameOfSymbol(ts::cpp::language(), Node.symbol());
      if (SymbolName == ts::cpp::symbols::sym_comment.name) {
        continue;
      }
      // currently, in header we only resolve declaration related inclusion
      // resolution, as function definition may change frequently
      if (SymbolName.find("declaration") == std::string::npos) {
        return false;
      }
    }

    std::shared_ptr<ts::Tree> TheirTree = Parser.parse(std::string(Their));
    ts::Node TheirRootNode = TheirTree->rootNode();
    for (ts::Node const &Node : TheirRootNode.namedChildren()) {
      std::string SymbolName =
          ts::Symbol::nameOfSymbol(ts::cpp::language(), Node.symbol());
      if (SymbolName == ts::cpp::symbols::sym_comment.name) {
        continue;
      }
      if (SymbolName.find("declaration") == std::string::npos) {
        return false;
      }
    }

    if (OurContain) {
      BRR.code = std::string(Our);
      BRR.desc = "Our side contains all the modifications, accept our side";
      spdlog::info("inclusion merged for conflict block[{}] in file[{}]",
                   BRR.index, CF.Filename);
      return true;
    }

    if (TheirContain) {
      BRR.code = std::string(Their);
      BRR.desc = "Their side contains all the modifications, accept their side";
      spdlog::info("inclusion merged for conflict block[{}] in file[{}]",
                   BRR.index, CF.Filename);
      return true;
    }
  }
  return false;
}

bool TextBasedHandler::doListMerge(std::string_view Our, std::string_view Their,
                                   ConflictFile const &CF,
                                   server::BlockResolutionResult &BRR) {
  std::vector<std::string> OurInclusions;
  bool OurAllInclusions =
      _details::parseInclusions(std::string(Our), OurInclusions);
  std::vector<std::string> TheirInclusions;
  bool TheirAllInclusions =
      _details::parseInclusions(std::string(Their), TheirInclusions);
  if (OurAllInclusions && TheirAllInclusions) {
    std::vector<std::string> Merged;
    if (_details::mergeVectors(OurInclusions, TheirInclusions, Merged)) {
      BRR.code = util::string_join(Merged, "");
      BRR.desc = "Headers merge.";
      spdlog::info("both sides of header include modified, we do a list merge "
                   "for conflict block[{}] in file[{}]",
                   BRR.index, CF.Filename);
      return true;
    }
  }

  // function definition or declaration
  // 抽取签名成为list，判断相交，合并
  if (_details::isCppHeader(CF.Filename)) {
    std::vector<std::string> OurDecls;
    if (!_details::parseDeclarations(std::string(Our), OurDecls)) {
      return false;
    }
    std::vector<std::string> TheirDecls;
    if (!_details::parseDeclarations(std::string(Their), TheirDecls)) {
      return false;
    }
    std::vector<std::string> Merged;
    if (_details::mergeVectors(OurDecls, TheirDecls, Merged)) {
      BRR.code = util::string_join(Merged, "\n");
      BRR.desc = "Declarations merge.";
      return true;
    }
    return false;
  }

  // cpp source, check function and merge function
  if (_details::isCppSource(CF.Filename)) {
    std::vector<std::string> OurDefinitions;
    if (!_details::parseDefinitions(std::string(Our), OurDefinitions)) {
      return false;
    }
    std::vector<std::string> TheirDefinitions;
    if (!_details::parseDefinitions(std::string(Their), TheirDefinitions)) {
      return false;
    }
    std::vector<std::string> Merged;
    if (_details::mergeVectors(OurDefinitions, TheirDefinitions, Merged)) {
      BRR.code = util::string_join(Merged, "\n");
      BRR.desc = "List merge.";
      spdlog::info("new feature added, we do a list merge for conflict "
                   "block[{}] in file[{}]",
                   BRR.index, CF.Filename);
      return true;
    }
    return false;
  }
  return false;
}

void TextBasedHandler::threeWayMerge(std::vector<ConflictFile> &ConflictFiles) {
  auto conflictsDir = fs::path(Meta.MSCacheDir) / "conflicts";
  fs::create_directories(conflictsDir);
  for (const ConflictFile &CF : ConflictFiles) {
    fs::path Relative = fs::relative(CF.Filename, Meta.ProjectPath);
    std::string ourFilePath = fs::path(Meta.MSCacheDir) /
                              magic_enum::enum_name(Side::OURS) / Relative;
    std::string baseFilePath = fs::path(Meta.MSCacheDir) /
                               magic_enum::enum_name(Side::BASE) / Relative;
    std::string theirFilePath = fs::path(Meta.MSCacheDir) /
                                magic_enum::enum_name(Side::THEIRS) / Relative;
    if (!exists(fs::path(ourFilePath)) || !exists(fs::path(baseFilePath)) ||
        !exists(fs::path(theirFilePath))) {
      continue;
    }
    auto CMD = fmt::format("git merge-file -p --diff3 {} {} {}", ourFilePath,
                           baseFilePath, theirFilePath);
    auto OutputOrErr = utils::ExecCommand(CMD, 10, 0, false);
    if (!OutputOrErr)
      spdlog::error("fail to merge {}, {} and {}", ourFilePath, baseFilePath,
                    theirFilePath);
    else {
      fs::path FilePath = conflictsDir / Relative;
      if (!exists(FilePath.parent_path()))
        fs::create_directories(FilePath.parent_path());
      util::file_overwrite_content(FilePath, OutputOrErr.get());
    }
  }
}

void TextBasedHandler::resolveConflictFiles(
    std::vector<ConflictFile> &ConflictFiles) {
  assert(ConflictFiles.size() &&
         "ConflictFile sizes should be greater than zero");

  spdlog::info("Resolving conflicts using heuristic based handler...");
  // TODO(hwa): squirrel bug, find why
  bool WithBase = false;
  if (ConflictFiles[0].ConflictBlocks.size() != 0) {
    std::string_view FirstCR = ConflictFiles[0].ConflictBlocks[0].ConflictRange;
    size_t OurPos = FirstCR.find(magic_enum::enum_name(ConflictMark::OURS));
    size_t BasePos = FirstCR.find(magic_enum::enum_name(ConflictMark::BASE));
    size_t TheirPos = FirstCR.find(magic_enum::enum_name(ConflictMark::THEIRS));
    size_t EndPos = std::string_view::npos;
    WithBase = OurPos != EndPos && BasePos != EndPos && TheirPos != EndPos;
  }

  threeWayMerge(ConflictFiles);
  bool NeedShrink = false;
  for (ConflictFile &CF : ConflictFiles) {
    bool EverResolved = false, AllResolved = true;
    spdlog::debug("resolving {}...", CF.Filename);

    std::vector<server::BlockResolutionResult> ResolvedBlocks;
    for (ConflictBlock &CB : CF.ConflictBlocks) {
      std::string_view OurCode, TheirCode;
      if (WithBase) {
        OurCode = extractCodeFromConflictRange(
            CB.ConflictRange, magic_enum::enum_name(ConflictMark::OURS),
            magic_enum::enum_name(ConflictMark::BASE));
        TheirCode = extractCodeFromConflictRange(
            CB.ConflictRange, magic_enum::enum_name(ConflictMark::THEIRS),
            magic_enum::enum_name(ConflictMark::END));
      } else {
        OurCode = extractCodeFromConflictRange(
            CB.ConflictRange, magic_enum::enum_name(ConflictMark::OURS),
            magic_enum::enum_name(ConflictMark::THEIRS));
        TheirCode = extractCodeFromConflictRange(
            CB.ConflictRange, magic_enum::enum_name(ConflictMark::THEIRS),
            magic_enum::enum_name(ConflictMark::END));
      }
      assert((!OurCode.empty() || !TheirCode.empty()) &&
             "at least one side of code should not be empty");
      server::BlockResolutionResult BRR;
      BRR.index = CB.Index;

      if (checkDeletion(OurCode, TheirCode, CF, BRR)) {
        ResolvedBlocks.push_back(std::move(BRR));
        CB.Resolved = true;
        continue;
      }

      if (checkOneSideDelta(OurCode, TheirCode, CF, BRR)) {
        ResolvedBlocks.push_back(std::move(BRR));
        CB.Resolved = true;
        continue;
      }

      if (checkInclusion(OurCode, TheirCode, CF, BRR)) {
        ResolvedBlocks.push_back(std::move(BRR));
        CB.Resolved = true;
        continue;
      }

      if (doListMerge(OurCode, TheirCode, CF, BRR)) {
        ResolvedBlocks.push_back(std::move(BRR));
        CB.Resolved = true;
        continue;
      }
    }

    std::for_each(CF.ConflictBlocks.begin(), CF.ConflictBlocks.end(),
                  [&](ConflictBlock const &CB) {
                    if (CB.Resolved) {
                      EverResolved = true;
                    } else {
                      AllResolved = false;
                    }
                  });
    // update resolved flag of ConflictFile
    CF.Resolved = AllResolved;
    // check any of blocks is resolved, if true, marshal resolution results to
    // MSCache dir; if false, do nothing
    if (EverResolved) {
      const std::string RelativePath =
          fs::relative(CF.Filename, Meta.ProjectPath).string();
      fs::path ResolutionDest =
          fs::path(Meta.MSCacheDir) / "resolutions" / pathToName(RelativePath);
      marshalResolutionResult(ResolutionDest.string(), RelativePath,
                              ResolvedBlocks);
    }

    // tidy up conflict files and their conflict blocks
    if (EverResolved) {
      NeedShrink = true;
    }
  }
  if (NeedShrink) {
    tidyUpConflictFiles(ConflictFiles);
  }
}

bool TextBasedHandler::checkDeletion(std::string_view Our,
                                     std::string_view Their,
                                     const ConflictFile &CF,
                                     server::BlockResolutionResult &BRR) {
  if (!Our.empty() && !Their.empty())
    return false;
  using namespace llvm;
  fs::path Relative = fs::relative(CF.Filename, Meta.ProjectPath);
  fs::path ConflictFile = fs::path(Meta.MSCacheDir) / "conflicts" / Relative;
  llvm::ErrorOr<std::unique_ptr<MemoryBuffer>> FileOrErr =
      MemoryBuffer::getFile(ConflictFile.string(), false, false);
  if (auto Err = FileOrErr.getError()) {
    spdlog::info("fail to read conflict file [{}], err message: {}",
                 ConflictFile.string(), Err.message());
    return false;
  }

  std::unique_ptr<MemoryBuffer> File = std::move(FileOrErr.get());
  std::string CodeRange = _details::extractConflictBlock(
      BRR.index, File->getBufferStart(), File->getBufferSize());
  if (CodeRange.empty())
    return false;
  std::string_view OurCode = extractCodeFromConflictRange(
      CodeRange, magic_enum::enum_name(ConflictMark::OURS),
      magic_enum::enum_name(ConflictMark::BASE));
  std::string_view BaseCode = extractCodeFromConflictRange(
      CodeRange, magic_enum::enum_name(ConflictMark::BASE),
      magic_enum::enum_name(ConflictMark::THEIRS));
  std::string_view TheirCode = extractCodeFromConflictRange(
      CodeRange, magic_enum::enum_name(ConflictMark::THEIRS),
      magic_enum::enum_name(ConflictMark::END));
  if (Our.empty()) {
    BRR.code = deletionOrModification(Their);
    BRR.desc = "Single side deletion.";
    spdlog::info("deletion detected for conflict block [{}] in file [{}]",
                 BRR.index, CF.Filename);
    return true;
  }
  if (Their.empty()) {
    BRR.code = deletionOrModification(Our);
    BRR.desc = "Single side deletion.";
    spdlog::info("deletion detected for conflict block [{}] in file [{}]",
                 BRR.index, CF.Filename);
    return true;
  }
  return false;
}

void TextBasedHandler::initDeletionInfavor() {
  const char *env = std::getenv("MERGEBOT_DELETION_INFAVOR");
  if (env == nullptr) {
    DeletionInfavor = DeletionInfavor::DELETION;
    return;
  }

  std::string env_str = std::string(env);
  std::transform(env_str.begin(), env_str.end(), env_str.begin(), ::tolower);
  if (env_str == "modification") {
    DeletionInfavor = DeletionInfavor::MODIFICATION;
  } else {
    DeletionInfavor = DeletionInfavor::DELETION;
  }
}

std::string TextBasedHandler::deletionOrModification(std::string_view code) {
  if (code.empty()) {
    return "";
  }

  switch (DeletionInfavor) {
  case DeletionInfavor::DELETION:
    return "";
  case DeletionInfavor::MODIFICATION:
    return std::string(code);
  }
}

// bool TextBasedHandler::checkDeletion(std::string_view Our,
//                                      std::string_view Their,
//                                      const ConflictFile &CF,
//                                      server::BlockResolutionResult &BRR) {
//   if (!Our.empty() && !Their.empty())
//     return false;
//   using namespace llvm;
//   fs::path Relative = fs::relative(CF.Filename, Meta.ProjectPath);
//   fs::path BaseFile =
//       fs::path(Meta.MSCacheDir) / magic_enum::enum_name(Side::BASE) /
//       Relative;
//
//   ErrorOr<std::unique_ptr<MemoryBuffer>> FileOrErr =
//       MemoryBuffer::getFile(BaseFile.string());
//   if (auto Err = FileOrErr.getError()) {
//     spdlog::error("failed to extract conflict block for base side file[{}], "
//                   "err message: {}",
//                   BaseFile.string(), Err.message());
//     return false;
//   }
//   std::unique_ptr<MemoryBuffer> File = std::move(FileOrErr.get());
//
//   std::string ToLookup =
//       Our.empty() ? trim(std::string(Their)) : trim(std::string(Our));
//   bool FoundNonEmpty = _details::isConflictBlockInBuffer(
//       ToLookup, File->getBufferStart(), File->getBufferEnd(),
//       File->getBufferSize());
//   if (FoundNonEmpty) {
//     BRR.code = "";
//     BRR.desc = "Single side deletion.";
//     spdlog::info("deletion detected for conflict block[{}] in file[{}]",
//                  BRR.index, CF.Filename);
//     return true;
//   }
//   return false;
// }
} // namespace sa
} // namespace mergebot
