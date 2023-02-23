//
// Created by whalien on 23/02/23.
//

#ifndef MB_MB_EXCEPTIONS_H
#define MB_MB_EXCEPTIONS_H

#include <crow.h>

#include <exception>
#include <string>

#include "../utility.h"

namespace mergebot {
namespace server {
class AppBaseException : public std::exception {
 public:
  explicit AppBaseException(std::string code = "S0000",
                            std::string msg = "MBSA服务端运行时异常")
      : code_(code), msg_(msg) {}

  const char* what() const noexcept override { return msg_.c_str(); }

  void handle(crow::response& res) const {
    auto buf = fmt::memory_buffer ();
    fmt::format_to(std::back_inserter(buf),
                   u8R"({{"code": "{}", "errorMsg": "{}", "data": {}}})", code_, msg_, nullptr);
    auto body = to_string(buf);
    auto status = crow::status::OK;
    if (msg_.length() && msg_[0] == 'C') {
      status = crow::status::BAD_REQUEST;
    }
    crow::response errRes(status, body);
    errRes.set_header("Content-Type", "application/json");
    res = std::move(errRes);
    res.end();
  }

 private:
  std::string code_;
  std::string msg_;
};
}  // namespace server
}  // namespace mergebot

#endif  // MB_MB_EXCEPTIONS_H
