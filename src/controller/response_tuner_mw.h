//
// Created by whalien on 23/02/23.
//

#ifndef MB_RESPONSE_CRAFTER_MW_H
#define MB_RESPONSE_CRAFTER_MW_H

#include <crow.h>
#include <fmt/format.h>

namespace mergebot{
namespace server {
struct ResponseCrafter
{
  struct context {};

  void before_handle(crow::request& req, crow::response& res, context& ctx){

  }

  void after_handle(crow::request& req, crow::response& res, context& ctx) {
    // response whose body is built with ResultEnum
    const auto body = res.body;
    crow::json::rvalue bodyRv = crow::json::load(body);
    if (!bodyRv.has("errorMsg"))    return;
    auto errorMsg = static_cast<std::string>(bodyRv["errorMsg"]);
    if (errorMsg.length() && errorMsg[0] == 'C') {
      res.code = crow::status::BAD_REQUEST;
    } else if (errorMsg.length() && errorMsg[0] == 'S') {
      res.code = crow::status::INTERNAL_SERVER_ERROR;
    }
    res.end();
  }
};
}  // namespace server
}  // namespace mergebot

#endif  // MB_RESPONSE_CRAFTER_MW_H
