//
// Created by whalien on 02/08/23.
//
#include "mergebot/lsp/communicator.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>

#include "mergebot/filesystem.h"
#include "mergebot/globals.h"

#define MB_DROP_CHILD_STDERR

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

  processId = fork();
  if (processId == -1) {
    spdlog::error("fail to fork: {}", strerror(errno));
    close(pipeIn[0]);
    close(pipeIn[1]);
    close(pipeOut[0]);
    close(pipeOut[1]);

    return nullptr;
  } else if (processId == 0) {
    // child process
    close(pipeIn[1]);
    close(pipeOut[0]);
    int ret1 = dup2(pipeIn[0], STDIN_FILENO);
    int ret2 = dup2(pipeOut[1], STDOUT_FILENO);
    if (ret1 == -1 || ret2 == -1) {
      perror("dup2");
      assert("failed to dup2" && false);
    }

#ifdef MB_DROP_CHILD_STDERR
    std::string logFileName = "/dev/null";
#else
    std::string logFileName = (fs::temp_directory_path() /
                               fmt::format("mergebot-{}-stderr.log", getpid()))
                                  .string();
#endif
    //    fs::create_directories(LOG_FOLDER);
    int logFd = open(logFileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (logFd == -1) {
      perror(logFileName.c_str());
      assert(false && "language server failed to open log file");
    }
    FILE* logStream = fdopen(logFd, "w");
    if (logStream == nullptr) {
      perror("fdopen");
      assert(false && "failed to convert file descriptor to FILE*");
    }

    // 重定向标准错误输出到logStream
    if (dup2(fileno(logStream), STDERR_FILENO) == -1) {
      perror("dup2");
      assert(false && "failed to dup2");
    }
    //    int ret3 = dup2(logFd, STDERR_FILENO);
    //    if (ret3 == -1) {
    //      perror("dup2");
    //      assert("failed to dup2" && false);
    //    }
    //    close(logFd);

    execl(executable, args, NULL);
    perror("execl");
    fclose(logStream);
    assert(false && fmt::format("failed to exec: {}", strerror(errno)).c_str());
  } else {
    // parent process
    close(pipeIn[0]);
    close(pipeOut[1]);
  }

  return std::unique_ptr<PipeCommunicator>(
      new PipeCommunicator(pipeIn, pipeOut, processId));
}

PipeCommunicator::PipeCommunicator(int* pipeIn, int* pipeOut, pid_t processId)
    : pipeIn{pipeIn[0], pipeIn[1]},
      pipeOut{pipeOut[0], pipeOut[1]},
      processId(processId) {
  spdlog::debug("&&&& pipeIn: {}, {}", pipeIn[0], pipeIn[1]);
  spdlog::debug("&&&& pipeOut: {}, {}", pipeOut[0], pipeOut[1]);
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
      spdlog::debug("child process was ended with a SIGTERM");
    } else {
      spdlog::debug("child process was ended with a {} signal",
                    WTERMSIG(status));
    }
  } else if (WIFEXITED(status)) {
    spdlog::debug("child process exited normally");
  }
  spdlog::debug("--------- pipe closed, {}, {}", pipeIn[1], pipeOut[0]);
  close(pipeIn[1]);
  close(pipeOut[0]);
}

ssize_t PipeCommunicator::write(const std::string& message) {
  constexpr size_t BufSize = 4096;
  size_t totalBytesWritten = 0;
  const char* BufStart = message.c_str();
  while (totalBytesWritten < message.size()) {
    ssize_t bytesWritten =
        ::write(pipeIn[1], BufStart + totalBytesWritten,
                std::min(BufSize, message.size() - totalBytesWritten));
    if (bytesWritten == -1) {
      spdlog::error("failed to write to pipe: {}", strerror(errno));
      return totalBytesWritten;
    }
    totalBytesWritten += bytesWritten;
  }
  return totalBytesWritten;
}

ssize_t PipeCommunicator::read(void* buf, size_t len) {
  int flags = fcntl(pipeOut[0], F_GETFL, 0);
  fcntl(pipeOut[0], F_SETFL, flags | O_NONBLOCK);

  return ::read(pipeOut[0], buf, len);

  size_t totalBytesRead = 0;
  ssize_t bytesRead = 0;
  char* bufferPtr = static_cast<char*>(buf);

  while (totalBytesRead < len) {
    bytesRead =
        ::read(pipeOut[0], bufferPtr + totalBytesRead, len - totalBytesRead);
    if (bytesRead > 0) {
      totalBytesRead += bytesRead;
    } else if (bytesRead == 0) {
      break;
    } else {
      // An error occurred
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      } else {
        return -1;
      }
    }
  }

  return totalBytesRead;
}
}  // namespace lsp
}  // namespace mergebot