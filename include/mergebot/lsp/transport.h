//
// Created by whalien on 11/06/23.
//

#ifndef MB_INCLUDE_MERGEBOT_LSP_TRANSPORT_H
#define MB_INCLUDE_MERGEBOT_LSP_TRANSPORT_H

#include <string_view>

#include "uri.h"

namespace mergebot {
namespace lsp {
using RequestID = std::string;
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

class JSONTransport : public Transport {
 public:
  const char *jsonrpc = "2.0";
  void notify(std::string_view method, json &params) override {
    json message = {
        {"jsonrpc", jsonrpc}, {"method", method}, {"params", params}};
    writeJSON(message);
  }
  void request(std::string_view method, json &params, RequestID &id) override {
    json message = {{"jsonrpc", jsonrpc},
                    {"id", id},
                    {"method", method},
                    {"params", params}};
    writeJSON(message);
  }

  [[noreturn]] int loop(MessageHandler &handler) override {
    while (true) {
      json value;
      if (readJSON(value)) {
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

 protected:
  virtual bool readJSON(json &) = 0;
  virtual bool writeJSON(json &) = 0;
};

// Returns a Transport that speaks JSON-RPC over a pair of streams.
// The input stream must be opened in binary mode.
// If InMirror is set, data read will be echoed to it.
//
// The use of C-style std::FILE* input deserves some explanation.
// Previously, std::istream was used. When a debugger attached on MacOS, the
// process received EINTR, the stream went bad, and clangd exited.
// A retry-on-EINTR loop around reads solved this problem, but caused clangd to
// sometimes hang rather than exit on other OSes. The interaction between
// istreams and signals isn't well-specified, so it's hard to get this right.
// The C APIs seem to be clearer in this respect.
// std::unique_ptr<Transport> newJSONTransport(std::FILE *In,
//                                            llvm::raw_ostream &Out,
//                                            llvm::raw_ostream *InMirror,
//                                            bool Pretty);

}  // namespace lsp
}  // namespace mergebot
#endif  // MB_INCLUDE_MERGEBOT_LSP_TRANSPORT_H
