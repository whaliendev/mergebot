//
// Created by whalien on 02/08/23.
//
#include "mergebot/lsp/communicator.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>

#include <random>

#include "mergebot/filesystem.h"
#include "mergebot/globals.h"

namespace mergebot {
namespace lsp {
std::unique_ptr<PipeCommunicator> PipeCommunicator::create(
    const char* executable, const char* args) {
  int pipeIn[2];
  int pipeOut[2];
  pid_t processId;

  if (pipe(pipeIn) == -1) {
    spdlog::error("failed to create pipe: {}", strerror(errno));
    return nullptr;
  }

  if (pipe(pipeOut) == -1) {
    spdlog::error("fail to create pipe: {}", strerror(errno));
    close(pipeIn[0]);
    close(pipeIn[1]);
    return nullptr;
  }

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distr(1000, 9999);  // 生成一个四位随机数

  int random_number = distr(gen);
  fs::create_directories(LOG_FOLDER);
  const std::string logFile =
      (fs::path(LOG_FOLDER) / fmt::format("child-stderr-{}.log", random_number))
          .string();
  int childLogFd = open(logFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (childLogFd == -1) {
    spdlog::error("fail to open child log file: {}", strerror(errno));
    close(pipeIn[0]);
    close(pipeIn[1]);
    close(pipeOut[0]);
    close(pipeOut[1]);
    return nullptr;
  }

  processId = fork();
  if (processId == -1) {
    spdlog::error("fail to fork: {}", strerror(errno));
    close(pipeIn[0]);
    close(pipeIn[1]);
    close(pipeOut[0]);
    close(pipeOut[1]);
    close(childLogFd);
    return nullptr;
  } else if (processId == 0) {
    // child process
    close(pipeIn[1]);
    close(pipeOut[0]);
    int ret1 = dup2(pipeIn[0], STDIN_FILENO);
    int ret2 = dup2(pipeOut[1], STDOUT_FILENO);
    int ret3 = dup2(childLogFd, STDERR_FILENO);
    if (ret1 == -1 || ret2 == -1 || ret3 == -1) {
      perror("failed to dup2");
      assert("failed to dup2" && false);
    }

    execl(executable, args, NULL);
    const std::string err_msg =
        fmt::format("failed to exec: {}", strerror(errno));
    ::write(STDERR_FILENO, err_msg.c_str(), err_msg.size());
    fsync(STDERR_FILENO);
    //    perror("failed to exec");
    assert(false && fmt::format("failed to exec: {}", strerror(errno)).c_str());
  } else {
    // parent process
    close(pipeIn[0]);
    close(pipeOut[1]);
    close(childLogFd);
  }

  return std::unique_ptr<PipeCommunicator>(
      new PipeCommunicator(pipeIn, pipeOut, processId));
}

PipeCommunicator::PipeCommunicator(int* pipeIn, int* pipeOut, pid_t processId)
    : pipeIn{pipeIn[0], pipeIn[1]},
      pipeOut{pipeOut[0], pipeOut[1]},
      processId(processId) {
  spdlog::info(">>>>>>> pipeIn: {}, {}", pipeIn[0], pipeIn[1]);
  spdlog::info(">>>>>>> pipeOut: {}, {}", pipeOut[0], pipeOut[1]);
}

PipeCommunicator::~PipeCommunicator() {
  // obviously double-close
  //  close(pipeOut[1]);
  //  close(pipeIn[0]);
  //  kill(processId, SIGTERM);

  int status;
  waitpid(processId, &status, 0);
  if (WIFSIGNALED(status)) {
    if (WTERMSIG(status) == SIGTERM) {
      spdlog::warn("child process was ended with a SIGTERM");
    } else {
      spdlog::warn("child process was ended with a {} signal",
                   WTERMSIG(status));
    }
  } else if (WIFEXITED(status)) {
    spdlog::debug("child process exited normally");
  }
  spdlog::info("<<<<<<< pipe closed, {}, {}", pipeIn[1], pipeOut[0]);
  close(pipeIn[1]);
  close(pipeOut[0]);
}

ssize_t PipeCommunicator::write(const std::string& message) {
  size_t totalBytesWritten = 0;
  constexpr size_t BufSize = 4096;
  constexpr size_t MaxRetries = 10;
  constexpr std::chrono::milliseconds RetryInterval(100);
  const char* BufStart = message.c_str();
  size_t retries = 0;

  while (totalBytesWritten < message.size()) {
    ssize_t bytesWritten =
        ::write(pipeIn[1], BufStart + totalBytesWritten,
                std::min(BufSize, message.size() - totalBytesWritten));

    if (bytesWritten >= 0) {
      totalBytesWritten += bytesWritten;
      retries = 0;  // 重置重试次数
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        if (++retries >= MaxRetries) {
          spdlog::error("max retries reached, failed to write to pipe");
          break;
        }
        spdlog::warn("write would block, retrying in {} ms",
                     RetryInterval.count());
        std::this_thread::sleep_for(RetryInterval);
      } else {
        spdlog::error("failed to write to pipe: {}", strerror(errno));
        break;
      }
    }
  }

  return totalBytesWritten;
}

/// \brief read from pipeOut[0] to buf with len bytes at most once a time and
/// return the bytes read in total
///
/// \param buf buf to read into
/// \param len max
/// bytes to read \return bytes read in total
ssize_t PipeCommunicator::read(void* buf, size_t len) {
  int flags = fcntl(pipeOut[0], F_GETFL, 0);
  fcntl(pipeOut[0], F_SETFL, flags | O_NONBLOCK);

  constexpr size_t BUF_SIZE = 4096;

  ssize_t totalBytesRead = 0;
  ssize_t bytesRead;

  while (totalBytesRead < len) {
    bytesRead =
        ::read(pipeOut[0], static_cast<char*>(buf) + totalBytesRead,
               std::min(static_cast<size_t>(BUF_SIZE), len - totalBytesRead));

    if (bytesRead == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No data available for non-blocking read, break the loop
        break;
      } else {
        // Handle other read errors if needed
        return -1;
      }
    } else if (bytesRead == 0) {
      // End of file, break the loop
      break;
    }

    totalBytesRead += bytesRead;
  }

  return totalBytesRead;
}

// ssize_t PipeCommunicator::read(void* buf, size_t len) {
//   int flags = fcntl(pipeOut[0], F_GETFL, 0);
//   fcntl(pipeOut[0], F_SETFL, flags | O_NONBLOCK);
//
//   return ::read(pipeOut[0], buf, len);
// }
}  // namespace lsp
}  // namespace mergebot