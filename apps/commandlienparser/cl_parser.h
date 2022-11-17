//
// Created by whalien on 17/11/22.
//

#ifndef MERGEBOT_CL_PARSER_H
#define MERGEBOT_CL_PARSER_H

namespace mb {
namespace clp {
enum ValueType {Required, Optional, NoValue};
enum Status { Parse_Exec, Parse_Ok, Parse_Error };
struct ParseStatus {
  Status error;
  Status status;
};

}   // end of namespace commandlineparser
}   // end of namespace mb

#endif  // MERGEBOT_CL_PARSER_H
