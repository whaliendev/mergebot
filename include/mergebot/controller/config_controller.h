//
// Created by whalien on 09/02/23.
//

#ifndef MB_CONFIG_CONTROLLER_H
#define MB_CONFIG_CONTROLLER_H

#include <crow.h>

#include "mergebot/server/result_vo_utils.h"

namespace mergebot {
namespace server {

mergebot::server::ResultVO PutConfiguration(const crow::request &req);

mergebot::server::ResultVO PostConfiguration(const crow::request &req);

}  // namespace server

}  // namespace mergebot

#endif  // MB_CONFIG_CONTROLLER_H
