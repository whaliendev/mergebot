//
// Created by whalien on 23/02/23.
//

#ifndef MB_MB_EXCEPTIONS_H
#define MB_MB_EXCEPTIONS_H

#include <crow.h>

#include <exception>
#include <string>

#include "mergebot/server/server_utility.h"

namespace mergebot {
namespace server {
class AppBaseException : public std::exception {
  template <typename... Args>
  using format_string_t = fmt::format_string<Args...>;

 public:
  explicit AppBaseException(mergebot::server::Result err)
      : code_(err.code), msg_(err.msg) {}

  AppBaseException(std::string code, std::string msg)
      : code_(code), msg_(msg) {}

  template <typename... Args>
  AppBaseException(std::string code, format_string_t<Args...> fmt,
                   Args&&... args)
      : code_(code), msg_(fmt::format(fmt, std::forward<Args>(args)...)) {}

  const char* what() const noexcept override { return msg_.c_str(); }

  void handle(crow::response& res) const {
    auto buf = fmt::memory_buffer();
    fmt::format_to(std::back_inserter(buf),
                   u8R"({{"code": "{}", "msg": "{}", "data": {}}})", code_,
                   msg_, "null");
    auto body = to_string(buf);
    auto status = crow::status::INTERNAL_SERVER_ERROR;
    if (code_.length() && code_[0] == 'C') {
      status = crow::status::BAD_REQUEST;
    } else if (code_.length() && code_[0] == 'S') {
      status = crow::status::INTERNAL_SERVER_ERROR;
    } else if (code_.length() && code_[0] == 'U') {
      status = crow::status::OK;
    }
    res.code = status;
    res.body = body;
    res.set_header("Content-Type", "application/json");
    res.end();
  }

 private:
  std::string code_;
  std::string msg_;
};
}  // namespace server
}  // namespace mergebot

#endif  // MB_MB_EXCEPTIONS_H
