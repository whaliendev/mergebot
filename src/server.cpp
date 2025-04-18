//
// Created by whalien on 09/02/23.
//

#ifndef NDEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#endif

#include <crow/app.h>
#include <crow/middlewares/cors.h>
#include <spdlog/async.h>
#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <thread>

#include "mergebot/controller/project_controller.h"
#include "mergebot/controller/resolve_controller.h"
#include "mergebot/core/sa_utility.h"
#include "mergebot/filesystem.h"
#include "mergebot/globals.h"
#include "mergebot/server/CrowSubLogger.h"
#include "mergebot/server/server_utility.h"
#include "mergebot/utils/ThreadPool.h"
#include "mergebot/utils/pathop.h"

namespace server = mergebot::server;

constexpr int M = 1024 * 1024;
constexpr double WORKLOAD = 0.75;
constexpr int TASK_QUEUE_SIZE = 1024;
static int DESTROYED = 0;

void ConfigBPRoutes(crow::Blueprint& bp);
void InitLogger();
void InitMergebot();

int main() {
  InitLogger();

  InitMergebot();

  // Init Server
  // substitute default logger of crow
  mergebot::CrowSubLogger subLogger;
  crow::logger::setHandler(&subLogger);

  crow::App<crow::CORSHandler> app;
  crow::Blueprint subApiBP("api/sa");

  auto& cors = app.get_middleware<crow::CORSHandler>();

  cors.blueprint(subApiBP).origin("*").headers("*").methods(
      "POST"_method, "GET"_method, "PUT"_method, "DELETE"_method,
      "PATCH"_method, "OPTIONS"_method);

  ConfigBPRoutes(subApiBP);

  // handler for no route matching
  CROW_CATCHALL_ROUTE(app)
  ([](const crow::request& req, crow::response& res) {
    server::ResultVOUtil::return_error(res, server::ResultEnum::NO_ROUTE_MATCH);
  });

  app.register_blueprint(subApiBP);

  app.loglevel(crow::LogLevel::INFO).port(18080).run();

  return 0;
}

void InitLogger() {
  try {
    // output to console for debugging purpose
    auto consoleSink =
        std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>(
            spdlog::color_mode::always);
#ifdef NDEBUG
    consoleSink->set_level(spdlog::level::info);
#else
    consoleSink->set_level(spdlog::level::debug);
#endif
    consoleSink->set_pattern("[%Y-%m-%d %T] [%t] %^[%l]%$ %@: %v");

    // output to file for analysis purpose, there are at most 3 rotating log
    // files, with a file size limit of 1024M
    // TODO: refactor log file destination to ~/.local/logs/mergebot/<file>.log
    auto rotateFileSink =
        std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            (mergebot::fs::path(mergebot::LOG_FOLDER) / "mergebot.log")
                .string(),
            1024 * M, 3);
    rotateFileSink->set_level(spdlog::level::info);
    rotateFileSink->set_pattern("[%Y-%m-%d %T.%e] [%t] [%l] %@: %v");

    spdlog::sinks_init_list sinkList = {consoleSink, rotateFileSink};
    spdlog::init_thread_pool(4096, 1);
    auto defaultLogger = std::make_shared<spdlog::async_logger>(
        "async_logger", sinkList, spdlog::thread_pool(),
        spdlog::async_overflow_policy::overrun_oldest);

    // global logger level should be lower than two sinks
#ifndef NDEBUG
    defaultLogger->set_level(spdlog::level::debug);
#else
    defaultLogger->set_level(spdlog::level::info);
#endif

    spdlog::register_logger(defaultLogger);
    spdlog::set_default_logger(defaultLogger);
    spdlog::flush_on(spdlog::level::err);
  } catch (const spdlog::spdlog_ex& ex) {
    std::cout << "spdlog initialization failed: " << ex.what() << '\n';
    spdlog::shutdown();
    exit(1);
  }
}

void InitMergebot() {
  namespace fs = std::filesystem;
  // Create dir .mergebot.
  // We may need to generate default configuration at boot in the near future.
  // Note that "Home" is Unix based OS specific, on windows, use
  // `getenv("USERPROFILE")`
  fs::path mergebotDirPath = mergebot::util::toabs(mergebot::MBDIR);

  try {
    // If directory exists, remove it recursively
    if (fs::exists(mergebotDirPath)) {
      spdlog::warn("Directory {} exists, removing it...",
                   mergebotDirPath.string());
      fs::remove_all(mergebotDirPath);
    }

    // Create the new directory
    fs::create_directory(mergebotDirPath);
    spdlog::info("Directory {} created successfully", mergebotDirPath.string());

  } catch (const std::exception& ex) {
    spdlog::error("Failed to init mergebot, reason: {}", ex.what());
    exit(1);
  }
}

void ConfigBPRoutes(crow::Blueprint& bp) {
  // initially, post configuration
  CROW_BP_ROUTE(bp, "/config/add")
      .methods(crow::HTTPMethod::POST)(
          [](const crow::request& req) { return "add initial configuration"; });
  // change the default configuration
  CROW_BP_ROUTE(bp, "/config/patch")
      .methods(crow::HTTPMethod::PUT)(
          [](const crow::request& req) { return "patch configuration"; });
  // get the current configuration
  CROW_BP_ROUTE(bp, "/config/list")
      .methods(crow::HTTPMethod::GET)(
          [](const crow::request& req) { return "list configuration"; });

  CROW_BP_ROUTE(bp, "/ms").methods(crow::HTTPMethod::OPTIONS)(
      [](const crow::request& req) {
        return crow::response(crow::status::OK);
      });

  // post merge scenario information
  CROW_BP_ROUTE(bp, "/ms").methods(crow::HTTPMethod::POST)(
      [](const crow::request& req, crow::response& res) {
        server::PostMergeScenario(req, res);
      });

  CROW_BP_ROUTE(bp, "/resolve")
      .methods(crow::HTTPMethod::OPTIONS)([](const crow::request& req) {
        return crow::response(crow::status::OK);
      });

  // resolution result of specified file
  CROW_BP_ROUTE(bp, "/resolve")
      .methods(crow::HTTPMethod::POST)(
          [](const crow::request& req, crow::response& res) {
            server::GetFileResolution(req, res);
          });

  CROW_BP_ROUTE(bp, "/health")
      .methods(crow::HTTPMethod::OPTIONS)([](const crow::request& req) {
        return crow::response(crow::status::OK);
      });

  CROW_BP_ROUTE(bp, "/health")
      .methods(crow::HTTPMethod::GET)([](const crow::request& req) {
        return crow::response(crow::status::OK, "OK");
      });
}
