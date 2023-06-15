//
// Created by whalien on 11/06/23.
//

#ifndef MB_INCLUDE_MERGEBOT_LSP_TRANSPORT_H
#define MB_INCLUDE_MERGEBOT_LSP_TRANSPORT_H

#include <nlohmann/json.hpp>
#include <string_view>

#include "uri.h"

namespace mergebot {
namespace lsp {
using RequestID = int32_t;
using json = nlohmann::json;
// Transport is responsible for maintaining the connection to a server
// application, and reading/writing structured messages to it.
//
// Transports have limited thread safety requirements:
//  - messages will not be sent concurrently
//  - messages MAY be sent while loop() is reading, or its callback is active
class Transport {
 public:
  virtual ~Transport() = default;

  // Called by lsp client to send messages to the server.
  virtual void notify(std::string_view method, json &Params) = 0;
  virtual void request(std::string_view method, json &params,
                       RequestID &id) = 0;

  // Implemented by lsp client to handle incoming messages. (See loop() below).
  class MessageHandler {
   public:
    MessageHandler() = default;
    virtual ~MessageHandler() = default;
    // Handler returns true to keep processing messages, or false to shut down.
    virtual bool onNotify(std::string_view method, json &params) = 0;
    virtual bool onRequest(std::string_view method, json &params, json &id) = 0;
    virtual bool onResponse(json &id, json &result) = 0;
    virtual bool onError(json &id, json &error) = 0;
  };
  // Called by lsp client to receive and send messages to the server.
  // The transport should in turn invoke the handler to process messages.
  // If handler returns false, the transport should immediately exit the loop.
  // (This is used to implement the `exit` notification).
  // Otherwise, it returns an error when the transport becomes unusable.
  virtual int loop(MessageHandler &) = 0;
};

class JSONTransport final : public Transport, noncopyable {
 public:
  const char *jsonrpc = "2.0";

 public:
  class JSONMessageReadWriter {
   public:
    /// read message from \p Transport
    /// \return the length of content part if success or 0 if error occurs
    virtual ssize_t readMessage(json &) = 0;
    /// write message to \p Transport
    /// \return the length of message written to \p Transport or 0 if error
    /// occurs
    virtual ssize_t writeMessage(json &) = 0;
    virtual ~JSONMessageReadWriter() noexcept = default;
  };

  static std::unique_ptr<JSONTransport> create(
      std::unique_ptr<JSONMessageReadWriter> jrw) {
    return std::unique_ptr<JSONTransport>(new JSONTransport(std::move(jrw)));
  }

  void notify(std::string_view method, json &params) override {
    json message = {
        {"jsonrpc", jsonrpc}, {"method", method}, {"params", params}};
    jrw->writeMessage(message);
  }
  void request(std::string_view method, json &params, RequestID &id) override {
    json message = {{"jsonrpc", jsonrpc},
                    {"id", id},
                    {"method", method},
                    {"params", params}};
    jrw->writeMessage(message);
  }

  [[noreturn]] int loop(MessageHandler &handler) override {
    while (true) {
      json value;
      if (jrw->readMessage(value)) {
        // ID may be any json value. If absent, this is a notification
        if (value.count("id")) {
          // https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#requestMessage
          if (value.contains("method")) {  // a request message
            handler.onRequest(value["method"].get<std::string>(),
                              value["params"], value["id"]);
          } else if (value.contains(
                         "result")) {  // a response message without error
            // https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#responseMessage
            handler.onResponse(value["id"], value["result"]);
          } else if (value.contains("error")) {  // an error response
            handler.onError(value["id"], value["error"]);
          }
        } else if (value.contains("method")) {  // notification
          if (value.contains("params")) {
            // https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#notificationMessage
            handler.onNotify(value["method"].get<std::string>(),
                             value["params"]);
          }
        }
      }
    }
  }

 private:
  std::unique_ptr<JSONMessageReadWriter> jrw;
  explicit JSONTransport(std::unique_ptr<JSONMessageReadWriter> jrw)
      : jrw(std::move(jrw)) {}
};

}  // namespace lsp
}  // namespace mergebot
#endif  // MB_INCLUDE_MERGEBOT_LSP_TRANSPORT_H
