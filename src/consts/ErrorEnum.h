//
// Created by whalien on 17/02/23.
//

#ifndef MB_ERRORENUM_H
#define MB_ERRORENUM_H

#include "../result_vo_utils.h"

namespace mergebot {
namespace ResultEnum {
const static server::ResultEnum NOT_A_GIT_REPO("U0001",
                                               "所传入项目路径非git仓库");
const static server::ResultEnum NO_CONFLICTS_FOUND("U0002",
                                                   "当前目录下无冲突文件");
}  // namespace ResultEnum
}  // namespace mergebot

#endif  // MB_ERRORENUM_H
