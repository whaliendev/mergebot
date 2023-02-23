//
// Created by whalien on 23/02/23.
//

#ifndef MB_EXCEPTIONHANDLER_H
#define MB_EXCEPTIONHANDLER_H
#include <crow.h>
#include <fmt/format.h>

namespace mergebot{
namespace server {
struct ExceptionHandler
{
  struct context {};

  void before_handle(crow::request& req, crow::response& res, context& ctx){

  }

  void after_handle(crow::request& req, crow::response& res, context& ctx) {
    if (res.code == crow::status::INTERNAL_SERVER_ERROR) {
      // format response body
      auto buf = fmt::memory_buffer();
      fmt::format_to(std::back_inserter(buf),
                     u8R"({{"code": "{}", "errorMsg": "{}", "data": {}}})", "S0000",
                     "MBSA服务端存在一个运行时异常，请检查日志", nullptr);
      auto body = to_string(buf);
      crow::response serverErrRes(crow::status::INTERNAL_SERVER_ERROR, body);
      serverErrRes.set_header("Content-Type", "application/json");
      res = std::move(serverErrRes);
      res.end();
    } else {
      // set status code based on the errorMsg
    }
  }
};
}
}

#endif  // MB_EXCEPTIONHANDLER_H
