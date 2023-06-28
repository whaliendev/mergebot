//
// Created by whalien on 28/06/23.
//
#include "mergebot/utils/fileio.h"

#include <spdlog/spdlog.h>

#include "mergebot/core/sa_utility.h"

namespace mergebot {
namespace util {
std::string file_get_content(std::string const &path) {
  std::ifstream fin(path);
  std::string content;
  std::istreambuf_iterator<char> iit(fin), eiit;
  std::back_insert_iterator<std::string> sit(content);
  std::copy(iit, eiit, sit);
  return content;
}

void file_put_content(std::string const &path, std::string const &content,
                      std::ios_base::openmode mode) {
  std::ofstream fout(path, mode);
  fout << content;
}

void file_overwrite_content(std::string const &path,
                            std::string const &content) {
  file_put_content(path, content, std::ios_base::out | std::ios_base::trunc);
}

bool file_put_binary(char const *arr_data, size_t arr_size,
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

bool copy_file(const std::string &source, const std::string &destination) {
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

bool file_overwrite_content_sync(const std::string &path,
                                 const std::string &content) {
  auto [fd, lck] = mergebot::utils::lockWRFD(path);
  if (fd == -1) {
    spdlog::error("fail to acquire write lock for file [{}]", path);
    return false;
  }
  ssize_t bytes_written = write(fd, content.c_str(), content.length());
  if (bytes_written == -1) {
    spdlog::error("fail to write to file [{}], reason: {}", path,
                  strerror(errno));
    mergebot::utils::unlockFD(path, fd, lck);
    close(fd);
    return false;
  }
  mergebot::utils::unlockFD(path, fd, lck);
  close(fd);
  return bytes_written == static_cast<ssize_t>(content.length());
}

std::optional<std::string> file_get_content_sync(const std::string &path) {
  auto [fd, lck] = mergebot::utils::lockRDFD(path);
  if (fd == -1) {
    spdlog::error("fail to acquire read lock for file [{}]", path);
    return std::nullopt;
  }
  std::string result;
  constexpr int buf_size = 4096;
  char buffer[buf_size];
  ssize_t bytes_read = -1;
  while ((bytes_read = read(fd, buffer, buf_size)) > 0) {
    result.append(buffer, bytes_read);
  }

  if (bytes_read == -1) {
    spdlog::error("fail to read from file [{}], reason: {}", path,
                  strerror(errno));
    mergebot::utils::unlockFD(path, fd, lck);
    close(fd);
    return std::nullopt;  // data read is dirty, drop it
  }
  mergebot::utils::unlockFD(path, fd, lck);
  close(fd);
  return result;
}
}  // namespace util
}  // namespace mergebot