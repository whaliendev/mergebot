//
// Created by whalien on 09/02/23.
//

#ifndef MB_PROJECT_CONTROLLER_H
#define MB_PROJECT_CONTROLLER_H

#include <crow/http_request.h>
#include <crow/http_response.h>

#include <shared_mutex>

#include "mergebot/server/result_vo_utils.h"

namespace mergebot {
namespace server {
void PostMergeScenario(const crow::request& req, crow::response& res);
}  // namespace server

}  // namespace mergebot

#endif  // MB_PROJECT_CONTROLLER_H
