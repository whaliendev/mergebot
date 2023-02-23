//
// Created by whalien on 23/02/23.
//

#ifndef MB_MB_EXCEPTIONS_H
#define MB_MB_EXCEPTIONS_H

#include <exception>
#include "../utility.h"

namespace mergebot {
namespace server {
class AppBaseException: public std::exception {
 public:
  AppBaseException()
};
}
}

#endif  // MB_MB_EXCEPTIONS_H
