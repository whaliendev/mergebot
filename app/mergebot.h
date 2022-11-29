//
// Created by whalien on 17/11/22.
//

#ifndef MERGEBOT_MERGEBOT_H
#define MERGEBOT_MERGEBOT_H

#include <stddef.h>

#include "commandlienparser/cl_parser.h"

class MergeBot {
 public:
  MergeBot();
  ~MergeBot();
  void exec();
  int exitCode() const { return exit_code_; }
  mergebot::commandlineparser::ParseStatus parse(size_t argc, char** argv);

 private:
  int exit_code_;
};

#endif  // MERGEBOT_MERGEBOT_H
