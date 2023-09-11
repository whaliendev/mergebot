//
// Created by whalien on 02/08/23.
//
#include "mergebot/lsp/communicator.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>

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

  processId = fork();
  if (processId == -1) {
    spdlog::error("fail to fork: {}", strerror(errno));
    close(pipeIn[0]);
    close(pipeIn[1]);
    close(pipeOut[0]);
    close(pipeOut[1]);

    return nullptr;
  } else if (processId == 0) {
    // Child process
    close(pipeIn[1]);
    close(pipeOut[0]);

    if (dup2(pipeIn[0], STDIN_FILENO) == -1 ||
        dup2(pipeOut[1], STDOUT_FILENO) == -1) {
      perror("Failed to dup2");
      exit(EXIT_FAILURE);
    }

    std::string logFileName =
        (fs::path(LOG_FOLDER) / fmt::format("child-{}-stderr.log", getpid()))
            .string();
    fs::create_directories(LOG_FOLDER);
    int logFd = open(logFileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (logFd == -1) {
      perror("Failed to open log file");
      exit(EXIT_FAILURE);
    }

    if (dup2(logFd, STDERR_FILENO) == -1) {
      perror("Failed to dup2 for STDERR");
      close(logFd);
      exit(EXIT_FAILURE);
    }
    close(logFd);

    execl(executable, args, NULL);
    perror("Failed to exec");
    exit(EXIT_FAILURE);
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
      processId(processId) {}

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

  close(pipeIn[1]);
  close(pipeOut[0]);
}

ssize_t PipeCommunicator::write(const std::string& message) {
  ssize_t bytesWritten = ::write(pipeIn[1], message.c_str(), message.size());
  if (bytesWritten == -1) {
    spdlog::error("failed to write to pipe: {}", strerror(errno));
  }
  return bytesWritten;
}

ssize_t PipeCommunicator::read(void* buf, size_t len) {
  int flags = fcntl(pipeOut[0], F_GETFL, 0);
  fcntl(pipeOut[0], F_SETFL, flags | O_NONBLOCK);

  return ::read(pipeOut[0], buf, len);
}
}  // namespace lsp
}  // namespace mergebot