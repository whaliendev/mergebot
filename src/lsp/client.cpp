//
// Created by whalien on 13/06/23.
//

#include "mergebot/lsp/client.h"

#include <shared_mutex>

#include "mergebot/lsp/protocol.h"

namespace mergebot {
namespace lsp {
ssize_t JSONRpcEndpoint::SendRequest(const RpcRequestBody& json) {
  std::string jsonStr = json.dump();
  std::string message = fillMessageHeader(jsonStr);
  // obtain a unique_lock for writing
  std::unique_lock<std::shared_mutex> lock(rwMutex);

  spdlog::debug("--> send request to server, message: {}", jsonStr);
  ssize_t bytesWritten = communicator->write(message);
  if (bytesWritten == -1) {
    spdlog::error("fail send request to server, error message: {}",
                  strerror(errno));
    return 0;
  }

  return bytesWritten;
}

std::optional<JSONRpcEndpoint::RpcResponseBody>
JSONRpcEndpoint::RecvResponse() {
  ssize_t bodySize = -1;
  while (true) {
    std::string line = readLine();
    if (line.empty()) {
      // empty pipe
      return "";
    }
    if (!util::ends_with(line, "\r\n")) {
      // illegal message format
      return std::nullopt;
    }

    line = line.substr(0, line.size() - 2);
    if (line.empty()) {
      break;
    } else if (util::starts_with(line, LEN_HEADER)) {
      line = line.substr(strlen(LEN_HEADER));
      char* end;
      bodySize = static_cast<ssize_t>(strtoll(line.c_str(), &end, 10));
    } else if (util::starts_with(line, TYPE_HEADER)) {
      spdlog::debug("content type line found: {}", line);
    } else {
      spdlog::error("bad header: unknown header");
    }
  }

  // illegal json message
  if (bodySize == -1) {
    return std::nullopt;
  }

  std::string msgContent = readMessageContent(bodySize);
  spdlog::debug("<-- response received, content part is {}", msgContent);
  json msgBodyJSON = json::parse(msgContent, nullptr, false);
  if (msgBodyJSON.is_discarded()) {
    spdlog::error("content part of response message is illegal: content is {}",
                  msgContent);
    return std::nullopt;
  }
  return msgBodyJSON;
}

std::string JSONRpcEndpoint::fillMessageHeader(const std::string& body) {
  std::ostringstream oss;
  oss << "Content-Length: " << body.size() << "\r\n\r\n" << body;
  return oss.str();
}

std::string JSONRpcEndpoint::readLine() {
  char buf[256];
  memset(buf, 0, sizeof(buf));
  size_t len = 0;
  ssize_t bytesRead = 0;
  {
    std::unique_lock<std::shared_mutex> lock(rwMutex);
    do {
      bytesRead = communicator->read(&buf[len], 1);
      if (bytesRead == -1) {
        if (errno == EAGAIN) {
          //          spdlog::debug("pipe is empty, return immediately");
          return "";
        }

        spdlog::error("fail to read from server, error message: {}",
                      strerror(errno));
        // data read in is dirty, drop them all
        return "";
      }
      if (buf[len] == '\n') {
        return std::string(buf);
      }
      len++;
    } while (bytesRead);
  }
  spdlog::info("the pipe is empty");
  return std::string(buf);
}

std::string JSONRpcEndpoint::readMessageContent(ssize_t len) {
  std::unique_lock<std::shared_mutex> lock(rwMutex);
  std::string content;
  content.resize(len);

  ssize_t bytesRead = communicator->read(content.data(), len);
  if (bytesRead == -1) {
    spdlog::error("unexpected error occurred: error message: {}",
                  strerror(errno));
    return "";
  }

  if (static_cast<size_t>(bytesRead) < len) {
    spdlog::error(
        "illegal language server response: bytesRead[{}] is less than message "
        "len[{}]",
        bytesRead, len);
    return "";
  }

  return content;
}

int LspEndpoint::ID = 0;

void LspEndpoint::SendNotification(std::string_view method,
                                   const JSONRpcParams& params) {
  sendMessage(method, params);
}

std::optional<LspEndpoint::JSONRpcResult> LspEndpoint::CallMethod(
    std::string_view method, const json& params) {
  int currentId = ID++;

  {
    std::unique_lock<std::mutex> lock(cvMutex);
    std::shared_ptr<std::condition_variable> cond =
        std::make_shared<std::condition_variable>();
    eventDict[currentId] = cond;

    sendMessage(method, params, currentId);
    if (shutdownFlag) {
      return std::nullopt;
    }

    if (cond->wait_for(lock, std::chrono::seconds(timeout)) ==
        std::cv_status::timeout) {
      spdlog::debug("timeout waiting for response, timeout is {}s", timeout);
      return std::nullopt;
    }

    // timeout or response received
    eventDict.erase(currentId);
  }

  auto it = responseDict.find(currentId);
  if (it != responseDict.end()) {
    auto& response = it->second;
    if (response.second.contains("code") &&
        response.second.contains("message")) {
      LSPError error(
          response.second["message"].get<std::string>(),
          static_cast<ErrorCode>(response.second["code"].get<int>()));
      spdlog::error("error occurs, response error: {}", error.toString());
      return std::nullopt;
    }
    return response.first;
  }
  spdlog::warn("cannot find rpcId in response dict, which is weird");
  return std::nullopt;
}

void LspEndpoint::Stop() { shutdownFlag = true; }

void LspEndpoint::operator()() {
  while (!shutdownFlag) {
    std::optional<JSONRpcEndpoint::RpcResponseBody> bodyOpt =
        rpcEndpoint->RecvResponse();
    if (!bodyOpt) {
      spdlog::error("recv response from server failed");
      break;
    }

    JSONRpcEndpoint::RpcResponseBody body = bodyOpt.value();

    if (body.empty()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    int rpcId = body.contains("id") ? body["id"].get<int>() : -1;
    std::string method =
        body.contains("method") ? body["method"].get<std::string>() : "";
    JSONRpcResult result = body.contains("result") ? body["result"] : "";
    JSONRpcParams params = body.contains("params") ? body["params"] : "";
    JSONRpcError error = body.contains("error") ? body["error"] : "";

    if (!method.empty()) {
      // notification or request
      if (rpcId != -1) {  // request
        // a call for method
        if (methodCallbacks.count(method)) {
          methodCallbacks[method](params);
        }
        SendResponse(rpcId, result, nullptr);
      } else {
        // a call for notification
        if (notifyCallbacks.count(method)) {
          notifyCallbacks[method](params);
        }
      }
    } else {
      handleResult(rpcId, result, error);
    }
  }
}

void LspEndpoint::sendMessage(std::string_view method, const json& params,
                              int id) {
  json value = {{"jsonrpc", jsonrpc},
                {"method", std::string(method)},
                {"params", params}};
  if (id != -1) {
    value["id"] = id;
  }
  rpcEndpoint->SendRequest(value);
}

void LspEndpoint::handleResult(int rpcId,
                               const LspEndpoint::JSONRpcResult& result,
                               const LspEndpoint::JSONRpcError& error) {
  if (rpcId == -1) return;
  std::unique_lock<std::mutex> lock(cvMutex);
  responseDict[rpcId] = std::make_pair(result, error);
  auto it = eventDict.find(rpcId);
  lock.unlock();
  if (it != eventDict.end()) {
    it->second->notify_one();
  }
}

void LspEndpoint::SendResponse(int id, const JSONRpcResult& result,
                               const JSONRpcError& error) {
  json value = {
      {"jsonrpc", jsonrpc}, {"id", id}, {"result", result}, {"error", error}};
  rpcEndpoint->SendRequest(value);
}
}  // namespace lsp
}  // namespace mergebot