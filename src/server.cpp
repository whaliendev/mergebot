//
// Created by whalien on 09/02/23.
//

#include "consts/ErrorEnum.h"
#include "controller/config_controller.h"
#include "controller/project_controller.h"
#include "controller/resolve_controller.h"
#include "crow/app.h"
#include "crow/middlewares/cors.h"
#include "utility.h"

namespace server = mergebot::server;

void ConfigBPRoutes(crow::Blueprint& bp);

int main() {
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

  app.register_blueprint(subApiBP);

  app.loglevel(crow::LogLevel::INFO).bindaddr("127.0.0.1").port(18080).run();
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
      .methods(crow::HTTPMethod::POST)([](const crow::request& req) {
        //            return server::PostProject(req);
        return mergebot::ResultVOUtil::error(
            mergebot::ResultEnum::NOT_A_GIT_REPO);
      });

  // post merge scenario information
  CROW_BP_ROUTE(bp, "/ms").methods(crow::HTTPMethod::POST)(
      [](const crow::request& req) { return server::PostMergeScenario(req); });

  // resolution result of specified file
  CROW_BP_ROUTE(bp, "/resolve")
      .methods(crow::HTTPMethod::POST)([](const crow::request& req) {
        return server::DoFileResolution(req);
      });
}
