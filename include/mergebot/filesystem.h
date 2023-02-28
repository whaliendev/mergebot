//
// Created by whalien on 27/02/23.
//

#ifndef MB_FILESYSTEM_H
#define MB_FILESYSTEM_H

#if __has_include(<filesystem>)
#include <filesystem>
namespace mergebot {
namespace fs = std::filesystem;
}
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace mergebot {
namespace fs = std::experimental::filesystem;
}
#else
#error "missing <filesystem> header."
#endif

#endif  // MB_FILESYSTEM_H
