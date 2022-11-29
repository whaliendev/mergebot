//
// Created by whalien on 17/11/22.
//

#ifndef MERGEBOT_COMMAND_LINE_PARSER_H
#define MERGEBOT_COMMAND_LINE_PARSER_H

namespace mergebot {
namespace commandlineparser {
enum ValueType { Required, Optional, NoValue };
enum Status { Parse_Exec, Parse_Ok, Parse_Error };
struct ParseStatus {
  Status error;
  Status status;
};

}  // namespace commandlineparser
}  // namespace mergebot

#endif  // MERGEBOT_COMMAND_LINE_PARSER_H
