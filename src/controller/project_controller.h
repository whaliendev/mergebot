//
// Created by whalien on 09/02/23.
//

#ifndef MB_PROJECT_CONTROLLER_H
#define MB_PROJECT_CONTROLLER_H

#include <crow/http_request.h>

#include "../result_vo_utils.h"

namespace mergebot {
namespace server {
ResultVO PostProject(const crow::request& req);
ResultVO PostMergeScenario(const crow::request& req);
}  // namespace server

}  // namespace mergebot

#endif  // MB_PROJECT_CONTROLLER_H
