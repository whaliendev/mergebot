//
// Created by whalien on 07/05/23.
//

#ifndef MB_INCLUDE_MERGEBOT_PARSER_FIELD_H
#define MB_INCLUDE_MERGEBOT_PARSER_FIELD_H

#include <tree_sitter/api.h>
extern "C" {
namespace mergebot {
namespace ts {
class Field {
 public:
  constexpr Field(TSFieldId id, const char* name) : id(id), name(name) {}

  operator TSFieldId() const { return id; }

  TSFieldId id;
  const char* name;
};
}  // namespace ts
}  // namespace mergebot
};

#endif  // MB_INCLUDE_MERGEBOT_PARSER_FIELD_H
