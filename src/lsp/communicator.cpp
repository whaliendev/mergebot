//
// Created by whalien on 02/08/23.
//
#include "mergebot/lsp/communicator.h"

#include <spdlog/spdlog.h>

#include <optional>

namespace mergebot {
namespace lsp {
PipeCommunicator::PipeCommunicator(const char* executable, const char* args) {
  if (pipe(pipeIn) == -1) {
    spdlog::error("Failed to create pipe: {}", strerror(errno));
    return;
  }

  if (pipe(pipeOut) == -1) {
    spdlog::error("Failed to create pipe: {}", strerror(errno));
    return;
  }

  processId = fork();
  if (processId == -1) {
    spdlog::error("Failed to fork: {}", strerror(errno));
    return;
  } else if (processId == 0) {
    // Child process
    close(pipeIn[1]);
    close(pipeOut[0]);
    dup2(pipeIn[0], STDIN_FILENO);
    dup2(pipeOut[1], STDOUT_FILENO);
    execl(executable, args, NULL);
    spdlog::error("failed to exec: {}", strerror(errno));
    exit(EXIT_FAILURE);
  } else {
    // Parent process
    close(pipeIn[0]);
    close(pipeOut[1]);
  }
}

PipeCommunicator::~PipeCommunicator() {
  close(pipeIn[1]);
  close(pipeOut[0]);
  close(pipeOut[1]);
  close(pipeIn[0]);
  int status;
  waitpid(processId, &status, 0);
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