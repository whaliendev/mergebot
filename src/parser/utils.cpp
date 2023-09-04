//
// Created by whalien on 04/09/23.
//

#include "mergebot/parser/utils.h"

#include <sstream>
#include <string>

#include "mergebot/parser/node.h"

namespace mergebot {
namespace ts {
std::string getTranslationUnitComment(const ts::Node &root) {
  std::stringstream comment;

  for (ts::Node const &child : root.children) {
    if (child.isNamed() && child.type() == "comment") {
      comment << child.text() << "\n";
    } else {
      break;
    }
  }
  return comment.str();
}

std::string getNodeComment(const Node &node) {
  std::stringstream comment;
  std::optional<ts::Node> previous = node.prevSibling();
  if (!previous.has_value() || !previous.value().isNamed() ||
      previous.value().type() != "comment") {
    return "";
  }
  std::vector<std::string> previousComments;
  while (previous.has_value() && previous.value().isNamed() &&
         previous.value().type() == "comment") {
    previousComments.push_back(previous.value().text());
    previous = previous.value().prevSibling();
  }

  for (auto it = previousComments.rbegin(); it != previousComments.rend();
       ++it) {
    comment << *it << "\n";
  }

  return comment.str();
}
}  // namespace ts
}  // namespace mergebot