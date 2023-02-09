//
// Created by whalien on 09/02/23.
//

#ifndef MB_RESULT_VO_UTILS_H
#define MB_RESULT_VO_UTILS_H

#include "crow/returnable.h"

namespace mergebot {
namespace server {

class ResultVO : public crow::returnable {
  std::string dump() const override { return "dummy"; }

 public:
  ResultVO() : crow::returnable("application/json") {}
};

}  // namespace server
}  // namespace mergebot

#endif  // MB_RESULT_VO_UTILS_H
