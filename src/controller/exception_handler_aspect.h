//
// Created by whalien on 23/02/23.
//

#ifndef MB_EXCEPTIONHANDLERASPECT_H
#define MB_EXCEPTIONHANDLERASPECT_H

#include <exception>
#include <crow.h>
#include <fmt/format.h>

#include "app_exception.h"

namespace mergebot {
namespace server {
template <typename Func, typename Response = crow::response>
class ExceptionHandlerAspect {
 public:
  ExceptionHandlerAspect(Func f, Response& res) : func_(f), res_(res) {}

  template <typename... Args>
  auto operator()(Args... args) -> decltype(func_(args...)) {
    static_assert(
        std::is_default_constructible<decltype(func_(args...))>::value,
        "the call result's type of function passed in must be default "
        "constructible");
    decltype(func_(args...)) res;
    try {
      res = func_(args...);
    } catch (const AppBaseException& ex) {
      ex.handle(res_);
    } catch (const std::exception& ex) {
      auto buf = fmt::memory_buffer ();
      fmt::format_to(std::back_inserter(buf),
                     u8R"({{"code": "{}", "errorMsg": "{}", "data": {}}})", "S0000", ex.what(), nullptr);
      auto body = to_string(buf);
      crow::response errRes(crow::status::INTERNAL_SERVER_ERROR, body);
      errRes.set_header("Content-Type", "application/json");
      res_ = std::move(errRes);
      res_.end();
    }
    return res;
  }

 private:
  Func func_;
  Response& res_;
};
}  // namespace server
}  // namespace mergebot
#endif  // MB_EXCEPTIONHANDLERASPECT_H
