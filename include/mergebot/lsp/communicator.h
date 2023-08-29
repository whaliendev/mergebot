//
// Created by whalien on 02/08/23.
//

#ifndef MB_INCLUDE_MERGEBOT_LSP_COMMUNICATOR_H
#define MB_INCLUDE_MERGEBOT_LSP_COMMUNICATOR_H

#include <spdlog/spdlog.h>
#include <sys/unistd.h>
#include <sys/wait.h>

#include <optional>

namespace mergebot {

namespace lsp {
/**
 * @class Communicator
 * @brief An interface for communication methods.
 *
 * This class provides an interface for sending and receiving raw messages over
 * a communication method (e.g., pipe, socket).
 */
class Communicator {
 public:
  virtual ~Communicator() = default;

  virtual ssize_t write(const std::string& message) = 0;
  virtual ssize_t read(void* buf, size_t len) = 0;
};

/**
 * @class PipeCommunicator
 * @brief Implements the Communicator interface for inter-process communication
 * via pipes.
 *
 * This class provides methods for sending and receiving raw messages over a
 * pipe. It also handles the creation and destruction of the pipe.
 */
class PipeCommunicator final : public Communicator {
 public:
  /**
   * @brief Factory method to construct PipeCommunicator
   *
   * This method creates the pipes and forks a new process. The child process
   * executes the given executable with the given arguments.
   * \return pointer of constructed PipeCommunicator, if the pointer is nullptr,
   * it means the construction failed.
   */
  static std::unique_ptr<PipeCommunicator> create(const char* executable,
                                                  const char* args);

  /**
   * @brief Destructor for PipeCommunicator.
   *
   * This method closes the pipes and waits for the child process to exit.
   */
  ~PipeCommunicator() override;

  /**
   * @brief Writes a message to the input pipe.
   *
   * This method writes a string to the input pipe for the child process to
   * read.
   */
  ssize_t write(const std::string& message) override;

  /**
   * @brief Reads a message from the output pipe.
   *
   * This method reads a message from the output pipe that the child process has
   * written. It reads up to len bytes and stores them in buf.
   */
  ssize_t read(void* buf, size_t len) override;

 private:
  PipeCommunicator(int* pipeIn, int* pipeOut, pid_t processId);

  int pipeIn[2];
  int pipeOut[2];
  pid_t processId;
};
}  // namespace lsp
}  // namespace mergebot

#endif  // MB_INCLUDE_MERGEBOT_LSP_COMMUNICATOR_H
