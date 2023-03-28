//
// Created by whalien on 09/02/23.
//

#ifndef MB_RESOLVE_CONTROLLER_H
#define MB_RESOLVE_CONTROLLER_H

#include <crow/http_request.h>

#include "mergebot/server/result_vo_utils.h"

namespace mergebot {
namespace server {
ResultVO DoFileResolution(const crow::request& req);
}
}  // namespace mergebot

#endif  // MB_RESOLVE_CONTROLLER_H
