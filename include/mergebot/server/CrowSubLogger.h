//
// Created by whalien on 25/02/23.
//

#ifndef MB_CROW_SUB_LOGGER_H
#define MB_CROW_SUB_LOGGER_H
#include <crow.h>
#include <spdlog/spdlog.h>

namespace mergebot {
class CrowSubLogger final : public crow::ILogHandler {
 public:
  CrowSubLogger() {}
  void log(std::string message, crow::LogLevel level) override {
    if (level == crow::LogLevel::Debug)
      spdlog::debug(message);
    else if (level == crow::LogLevel::Info)
      spdlog::info(message);
    else if (level == crow::LogLevel::Warning)
      spdlog::warn(message);
    else if (level == crow::LogLevel::Error)
      spdlog::error(message);
    else if (level == crow::LogLevel::CRITICAL)
      spdlog::critical(message);
    else {
    }
  }
};
}  // namespace mergebot

#endif  // MB_CROW_SUB_LOGGER_H
