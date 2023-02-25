//
// Created by whalien on 09/02/23.
//
#include "project_controller.h"

#include <crow/http_response.h>
#include <crow/json.h>

#include <vector>

#include "../server/utility.h"
#include "exception_handler_aspect.h"

namespace mergebot {
namespace server {
bool __containKeys(const crow::json::rvalue& bodyJson,
                   const std::vector<std::string>& keys) {
  for (const auto& key : keys) {
    if (!bodyJson.has(key)) {
      return false;
    }
  }
  return true;
}

crow::json::wvalue __doPostProject(const crow::request& req,
                                   crow::response& res) {
  const auto body = crow::json::load(req.body);
  if (body.error() || !__containKeys(body, {"project", "path"})) {
    throw AppBaseException(ResultEnum::BAD_REQUEST);
  }
  crow::json::wvalue data = {{"patch", "ok"}};
  return data;
}

void PostProject(const crow::request& req, crow::response& res) {
  // 检查目录是否有效，检查是否为git仓库，检查是否为初次运行，生成唯一目录，映射文件
  auto internalPostProject =
      ExceptionHandlerAspect<CReqMResFuncType>(__doPostProject, res);
  auto rv = internalPostProject(req, res);
  if (rv.keys().size()) ResultVOUtil::return_success(res, rv);
}

void PostMergeScenario(const crow::request& req, crow::response& res) {
  ResultVO rv;
}

}  // namespace server
}  // namespace mergebot
