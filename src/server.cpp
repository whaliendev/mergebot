//
// Created by whalien on 09/02/23.
//

#include <crow/app.h>
#include <crow/middlewares/cors.h>
#include <spdlog/async.h>
#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

#include <memory>

#include "controller/project_controller.h"
#include "controller/resolve_controller.h"
#include "server/CrowSubLogger.h"
#include "server/utility.h"

namespace server = mergebot::server;

constexpr int M = 1024 * 1024;

void ConfigBPRoutes(crow::Blueprint& bp);
void InitLogger();

int main() {
  try {
    InitLogger();
  } catch (const spdlog::spdlog_ex& ex) {
    std::cout << "spdlog initialization failed: " << ex.what() << std::endl;
    spdlog::shutdown();
    exit(1);
  }

  // substitute default logger of crow
  mergebot::CrowSubLogger subLogger;
  crow::logger::setHandler(&subLogger);

  crow::App<crow::CORSHandler> app;
  auto& cors = app.get_middleware<crow::CORSHandler>();
  crow::Blueprint subApiBP("api/sa");

  cors.global()
      .headers("X-Custom-Header", "Upgrade-Insecure-Requests", "X-Requested-With")
      .origin("http://127.0.0.1:80")
      .methods(crow::HTTPMethod::GET, crow::HTTPMethod::POST, crow::HTTPMethod::PUT)
      .max_age(10000)
      .allow_credentials();

  ConfigBPRoutes(subApiBP);

  CROW_CATCHALL_ROUTE(app)
  ([](const crow::request& req, crow::response& res) {
    server::ResultVOUtil::return_error(res, server::ResultEnum::NO_ROUTE_MATCH);
  });

  app.register_blueprint(subApiBP);

  app.loglevel(crow::LogLevel::INFO).bindaddr("127.0.0.1").port(18080).run();
}

void InitLogger() {
  // output to console for debugging purpose
  auto consoleSink =
      std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>(spdlog::color_mode::always);
  consoleSink->set_level(spdlog::level::debug);
  consoleSink->set_pattern("%^[%Y-%H-%M %T] [%t] [%l] %@: %v%$");

  // output to file for analysis purpose, there are at most 3 rotating log
  // files, with a file size limit of 1024M
  // TODO: refactor log file destination to ~/.local/logs/mergebot/<file>.log
  auto rotateFileSink =
      std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/mergebot", 1024 * M, 3);
  rotateFileSink->set_level(spdlog::level::info);
  rotateFileSink->set_pattern("[%Y-%H-%M %T.%e] [%t] [%l] %@: %v");

  spdlog::sinks_init_list sinkList = {consoleSink, rotateFileSink};
  spdlog::init_thread_pool(4096, 1);
  auto defaultLogger =
      std::make_shared<spdlog::async_logger>("async_logger", sinkList, spdlog::thread_pool(),
                                             spdlog::async_overflow_policy::overrun_oldest);

  spdlog::register_logger(defaultLogger);
  spdlog::set_default_logger(defaultLogger);
}

void ConfigBPRoutes(crow::Blueprint& bp) {
  // initially, post configuration
  CROW_BP_ROUTE(bp, "/config/add").methods(crow::HTTPMethod::POST)([](const crow::request& req) {
    return "add initial configuration";
  });
  // change the default configuration
  CROW_BP_ROUTE(bp, "/config/patch").methods(crow::HTTPMethod::PUT)([](const crow::request& req) {
    return "patch configuration";
  });
  // get the current configuration
  CROW_BP_ROUTE(bp, "/config/list").methods(crow::HTTPMethod::GET)([](const crow::request& req) {
    return "list configuration";
  });

  // project specific
  CROW_BP_ROUTE(bp, "/project")
      .methods(crow::HTTPMethod::POST)(
          [](const crow::request& req, crow::response& res) { server::PostProject(req, res); });

  // post merge scenario information
  CROW_BP_ROUTE(bp, "/ms").methods(crow::HTTPMethod::POST)(
      [](const crow::request& req, crow::response& res) { server::PostMergeScenario(req, res); });

  // resolution result of specified file
  CROW_BP_ROUTE(bp, "/resolve").methods(crow::HTTPMethod::POST)([](const crow::request& req) {
    return server::DoFileResolution(req);
  });
}
