//
// Created by whalien on 27/02/23. This file is taken from
// [zeno](https://github.com/zenustech/zeno) no change made to it.
//

#ifndef MB_FILEIO_H
#define MB_FILEIO_H

#include <cstdio>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "mergebot/filesystem.h"

namespace mergebot {
namespace util {
[[nodiscard]] std::string file_get_content(std::string const &path);

void file_put_content(std::string const &path, std::string const &content,
                      std::ios_base::openmode mode);

void file_overwrite_content(std::string const &path,
                            std::string const &content);

[[nodiscard]] bool file_overwrite_content_sync(std::string const &path,
                                               std::string const &content);

[[nodiscard]] std::optional<std::string> file_get_content_sync(
    std::string const &path);

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

bool file_put_binary(char const *arr_data, size_t arr_size,
                     std::string const &path);

template <class Arr = std::vector<char>>
static bool file_put_binary(Arr const &arr, std::string const &path) {
  return file_put_binary(std::data(arr), std::size(arr), path);
}

bool copy_file(const std::string &source, const std::string &destination);

/// \brief Compute the offsets of each line in a file
/// \param filename the file to compute the offsets for
/// \return a vector of offsets, where the first element is the offset of the
/// 0th line
std::vector<size_t> compute_offsets(std::string_view filename);

/// \brief Read a chunk of a file
/// \param filename the file to read from
/// \param start the offset of the first line to read
/// \param offset the number of lines to read
/// \param offsets the offsets of each line in the file
/// \return the chunk of the file
std::string read_file_chunk(std::string_view filename, size_t start,
                            size_t offset, const std::vector<size_t> &offsets);
}  // namespace util
}  // namespace mergebot

#endif  // MB_FILEIO_H
