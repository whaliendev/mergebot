//
// Created by whalien on 09/02/23.
//

#ifndef MB_RESULT_VO_UTILS_H
#define MB_RESULT_VO_UTILS_H
#include <crow/http_response.h>
#include <crow/json.h>
#include <crow/returnable.h>
#include <fmt/format.h>

#include <iomanip>
#include <nlohmann/json.hpp>

namespace mergebot {
namespace server {
struct Result {
  std::string code{"00000"};
  std::string msg{""};

  Result(std::string code, std::string msg) : code(code), msg(msg) {}
};

struct ResultVO : public crow::returnable {
  std::string code{"00000"};
  std::string msg{""};
  crow::json::wvalue data{nullptr};

  ResultVO() : crow::returnable("application/json") {}

  ResultVO(std::string code, std::string msg) : ResultVO(code, msg, nullptr) {}

  ResultVO(std::string code, std::string msg, crow::json::wvalue data)
      : crow::returnable("application/json"),
        code(code),
        msg(msg),
        data(std::move(data)) {}

  std::string dump() const override {
    auto buf = fmt::memory_buffer();
    fmt::format_to(std::back_inserter(buf),
                   u8R"({{"code": "{}", "msg": "{}", "data": {}}})", code, msg,
                   data.dump());
    return to_string(buf);
  }
};
}  // namespace server
}  // namespace mergebot

#endif  // MB_RESULT_VO_UTILS_H
