//
// Created by whalien on 27/02/23. This file is taken from
// [zeno](https://github.com/zenustech/zeno) no change made to it.
//

#ifndef MB_FILEIO_H
#define MB_FILEIO_H

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "mergebot/filesystem.h"

namespace mergebot {
namespace util {
[[nodiscard]] static std::string file_get_content(std::string const &path) {
  std::ifstream fin(path);
  std::string content;
  std::istreambuf_iterator<char> iit(fin), eiit;
  std::back_insert_iterator<std::string> sit(content);
  std::copy(iit, eiit, sit);
  return content;
}

static void file_put_content(std::string const &path,
                             std::string const &content,
                             std::ios_base::openmode mode) {
  std::ofstream fout(path, mode);
  fout << content;
}

static void file_overwrite_content(std::string const &path,
                                   std::string const &content) {
  file_put_content(path, content, std::ios_base::out | std::ios_base::trunc);
}

static bool file_exists(std::string const &path) {
  std::ifstream fin(path);
  return static_cast<bool>(fin);
}

template <class Arr = std::vector<char>>
static Arr file_get_binary(std::string const &path) {
  char const *filename = path.c_str();
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    perror(filename);
    return {};
  }
  if (fseek(fp, 0, SEEK_END) != 0) {
    perror(filename);
    fclose(fp);
    return {};
  }
  long size = ftell(fp);
  if (size == -1) {
    perror(filename);
    fclose(fp);
    return {};
  }
  rewind(fp);
  Arr res;
  res.resize(size);
  size_t n = fread(res.data(), res.size(), 1, fp);
  if (n != 1) {
    perror(filename);
  }
  fclose(fp);
  return res;
}

static bool file_put_binary(char const *arr_data, size_t arr_size,
                            std::string const &path) {
  char const *filename = path.c_str();
  FILE *fp = fopen(filename, "wb");
  if (!fp) {
    perror(filename);
    return {};
  }
  if (fseek(fp, 0, SEEK_END) != 0) {
    perror(filename);
    fclose(fp);
    return {};
  }
  long size = ftell(fp);
  if (size == -1) {
    perror(filename);
    fclose(fp);
    return {};
  }
  rewind(fp);
  size_t n = fwrite(arr_data, arr_size, 1, fp);
  if (n != 1) {
    perror(filename);
  }
  fclose(fp);
  return true;
}

template <class Arr = std::vector<char>>
static bool file_put_binary(Arr const &arr, std::string const &path) {
  return file_put_binary(std::data(arr), std::size(arr), path);
}

static bool copy_file(const std::string &source,
                      const std::string &destination) {
  int srcFd = open(source.c_str(), O_RDONLY);
  if (srcFd == -1) {
    spdlog::error("failed to open source file: {}", source);
    return false;
  }

  // create parent folder of the destination file if it doesn't exist
  std::string parentDir = fs::path(destination).parent_path();
  if (mkdir(parentDir.c_str(), 0777) == -1 && errno != EEXIST) {
    spdlog::error("failed to create parent directory: {}", parentDir);
    close(srcFd);
    return false;
  }

  int destFd = open(destination.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
                    S_IRUSR | S_IWUSR);
  if (destFd == -1) {
    spdlog::error("failed to open destination file: {}", destination);
    close(srcFd);
    return false;
  }

  const int bufferSize = 4096;
  char buffer[bufferSize];
  ssize_t bytesRead;

  while ((bytesRead = read(srcFd, buffer, bufferSize)) > 0) {
    ssize_t bytesWritten = write(destFd, buffer, bytesRead);
    if (bytesWritten == -1) {
      spdlog::error("failed to write to destination file: {}", destination);
      close(srcFd);
      close(destFd);
      return false;
    }
  }

  close(srcFd);
  close(destFd);

  return true;
}
}  // namespace util
}  // namespace mergebot

#endif  // MB_FILEIO_H
