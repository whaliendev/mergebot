//
// Created by whalien on 23/02/23.
//

#ifndef MB_EXCEPTION_HANDLER_ASPECT_H
#define MB_EXCEPTION_HANDLER_ASPECT_H

#include <crow.h>
#include <fmt/format.h>

#include <exception>

#include "app_exception.h"

namespace mergebot {
namespace server {

using CReqMResFuncType = std::function<crow::json::wvalue(const crow::request&, crow::response&)>;

template <typename Func, typename Response = crow::response>
class ExceptionHandlerAspect {
 private:
  Func func_;
  Response& res_;

 public:
  ExceptionHandlerAspect(Func f, Response& res) : func_(f), res_(res) {}

  /// a global exception handling wrapper
  /// \tparam Args parameters pack
  /// \param args parameters passed to internal @code func_ @endcode
  /// \return Most of the time, the return value's type is `crow::json::wvalue`. Default constructed
  /// wvalue and wvalue initialized with nullptr or {} has the same internal type
  /// `crow::json::type::Object`. However, nullptr or {} is a valid response, while default
  /// constructed wvalue is not. Thus, the default constructed wvalue should be
  /// modified to a form we can distinguish these two situations.
  /// @see{src/server/utility.h}
  template <typename... Args>
  auto operator()(Args&&... args) -> decltype(func_(std::forward<Args>(args)...)) {
    static_assert(
        std::is_default_constructible<decltype(func_(std::forward<Args>(args)...))>::value,
        "the call return value of function passed in must be default constructible");
    decltype(func_(std::forward<Args>(args)...)) res({{"error", true}});
    try {
      res = func_(std::forward<Args>(args)...);
    } catch (const AppBaseException& ex) {
      ex.handle(res_);
    } catch (const std::exception& ex) {
      auto buf = fmt::memory_buffer();
      fmt::format_to(std::back_inserter(buf), u8R"({{"code": "{}", "errorMsg": "{}", "data": {}}})",
                     "S0000", ex.what(), nullptr);
      auto body = to_string(buf);
      res_.code = crow::status::INTERNAL_SERVER_ERROR;
      res_.body = body;
      res_.set_header("Content-Type", "application/json");
      res_.end();
    }
    return res;
  }
};
}  // namespace server
}  // namespace mergebot
#endif  // MB_EXCEPTION_HANDLER_ASPECT_H
