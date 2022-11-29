#include <utils/String.h>

#include "clp/command_line_parser.h"
#include "mergebot.h"

int main(int argc, char** argv) {
  MergeBot mb;
  const mergebot::commandlineparser::ParseStatus status = mb.parse(argc, argv);
}
