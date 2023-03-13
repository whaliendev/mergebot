//
// Created by whalien on 09/02/23.
//

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

#include "controller/project_controller.h"
#include "controller/resolve_controller.h"
#include "globals.h"
#include "mergebot/utils/ThreadPool.h"
#include "mergebot/utils/pathop.h"
#include "server/CrowSubLogger.h"
#include "utility.h"

namespace server = mergebot::server;

constexpr int M = 1024 * 1024;
constexpr int WORKLOAD = 0.75;
constexpr int TASK_QUEUE_SIZE = 1024;
static int DESTROYED = 0;

void ConfigBPRoutes(crow::Blueprint& bp);
void InitLogger();
void InitMergebot();
void InitThreadPool();

[[noreturn]] int main() {
  InitLogger();

  InitMergebot();

  InitThreadPool();
  // Init Server
  // substitute default logger of crow
  mergebot::CrowSubLogger subLogger;
  crow::logger::setHandler(&subLogger);

  crow::App<crow::CORSHandler> app;
  auto& cors = app.get_middleware<crow::CORSHandler>();
  crow::Blueprint subApiBP("api/sa");

  cors.global()
      .headers("X-Custom-Header", "Upgrade-Insecure-Requests",
               "X-Requested-With")
      .origin("http://127.0.0.1:80")
      .methods(crow::HTTPMethod::GET, crow::HTTPMethod::POST,
               crow::HTTPMethod::PUT)
      .max_age(10000)
      .allow_credentials();

  ConfigBPRoutes(subApiBP);

  // handler for no route matching
  CROW_CATCHALL_ROUTE(app)
  ([](const crow::request& req, crow::response& res) {
    server::ResultVOUtil::return_error(res, server::ResultEnum::NO_ROUTE_MATCH);
  });

  app.register_blueprint(subApiBP);

  app.loglevel(crow::LogLevel::INFO).bindaddr("127.0.0.1").port(18080).run();
}

void InitThreadPool() {
  uint coreNum = std::thread::hardware_concurrency();
  if (!coreNum) coreNum = 6;
  int createRes =
      ThreadPool::threadpool_create(coreNum * WORKLOAD, TASK_QUEUE_SIZE);
  if (createRes != 0) {
    spdlog::error("failed to create global thread pool");
    exit(1);
  }

  auto poolSigDestroyer = [](int status) {
    if (!DESTROYED) {
      SPDLOG_INFO("destroying thread pool...");
      int err = ThreadPool::threadpool_destroy();
      if (!err)
        SPDLOG_ERROR(
            "fail to destroy thread pool, some resources may has not been "
            "released");
      DESTROYED = 1;
    }
    exit(status);
  };
  auto poolNorDestroyer = []() {
    if (!DESTROYED) {
      SPDLOG_INFO("destroying thread pool...");
      int err = ThreadPool::threadpool_destroy();
      if (!err)
        SPDLOG_ERROR(
            "fail to destroy thread pool, some resources may has not been "
            "released");
      DESTROYED = 1;
    }
  };
  // ^C
  signal(SIGINT, poolSigDestroyer);
  // abort
  signal(SIGABRT, poolSigDestroyer);
  // sent by kill command
  signal(SIGTERM, poolSigDestroyer);
  // ^Z
  signal(SIGTSTP, poolSigDestroyer);
  // return from main, or exit by exit function
  std::atexit(poolNorDestroyer);
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
    // FIX(hwa): there is something wrong, use spd::info instead.
    consoleSink->set_level(spdlog::level::debug);
#endif
    consoleSink->set_pattern("[%Y-%H-%M %T] [%t] %^[%l]%$ %@: %v");

    // output to file for analysis purpose, there are at most 3 rotating log
    // files, with a file size limit of 1024M
    // TODO: refactor log file destination to ~/.local/logs/mergebot/<file>.log
    auto rotateFileSink =
        std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/mergebot",
                                                               1024 * M, 3);
    rotateFileSink->set_level(spdlog::level::info);
    rotateFileSink->set_pattern("[%Y-%H-%M %T.%e] [%t] [%l] %@: %v");

    spdlog::sinks_init_list sinkList = {consoleSink, rotateFileSink};
    spdlog::init_thread_pool(4096, 1);
    auto defaultLogger = std::make_shared<spdlog::async_logger>(
        "async_logger", sinkList, spdlog::thread_pool(),
        spdlog::async_overflow_policy::overrun_oldest);

    spdlog::register_logger(defaultLogger);
    spdlog::set_default_logger(defaultLogger);
  } catch (const spdlog::spdlog_ex& ex) {
    std::cout << "spdlog initialization failed: " << ex.what() << std::endl;
    spdlog::shutdown();
    exit(1);
  }
}

void InitMergebot() {
  namespace fs = std::filesystem;
  // create dir .mergebot.
  // we may need to generate default configuration at boot in the near future.
  // Note that "Home" is Unix based OS specific, on windows, use
  // `getenv("USERPROFILE")`
  fs::path mergebotDirPath = mergebot::util::toabs(mergebot::MBDIR);
  if (fs::exists(mergebotDirPath)) return;
  try {
    fs::create_directory(mergebotDirPath);
  } catch (const std::exception& ex) {
    spdlog::error("failed to init mergebot, reason: {}", ex.what());
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

  // project specific
  CROW_BP_ROUTE(bp, "/project")
      .methods(crow::HTTPMethod::POST)(
          [](const crow::request& req, crow::response& res) {
            server::PostProject(req, res);
          });

  // post merge scenario information
  CROW_BP_ROUTE(bp, "/ms").methods(crow::HTTPMethod::POST)(
      [](const crow::request& req, crow::response& res) {
        server::PostMergeScenario(req, res);
      });

  // resolution result of specified file
  CROW_BP_ROUTE(bp, "/resolve")
      .methods(crow::HTTPMethod::POST)([](const crow::request& req) {
        return server::DoFileResolution(req);
      });
}
