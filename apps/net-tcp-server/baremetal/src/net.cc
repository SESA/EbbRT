//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <tcp_handler.hpp>

#include <ebbrt/StaticIOBuf.h>
#include <ebbrt/UniqueIOBuf.h>

class EchoReceiver : public TcpHandler {
 public:
  explicit EchoReceiver(ebbrt::NetworkManager::TcpPcb pcb)
      : TcpHandler(std::move(pcb)) {}

  // Callback to be invoked on a tcp segment receive event.
  void Receive(std::unique_ptr<ebbrt::MutIOBuf> buf) override {
    // This receiver just echos the data back
    Send(std::move(buf));
  }

  void Close() override { Shutdown(); }
  void Abort() override {}
};

class AllocingReceiver : public TcpHandler {
 public:
  explicit AllocingReceiver(ebbrt::NetworkManager::TcpPcb pcb)
      : TcpHandler(std::move(pcb)) {}

  void Receive(std::unique_ptr<ebbrt::MutIOBuf> buf) override {
    // Allocate a new buffer to hold 6 bytes
    auto new_buf = ebbrt::MakeUniqueIOBuf(6);
    // Fill it with the string
    strcpy(reinterpret_cast<char*>(new_buf->MutData()), "test\n");  // NOLINT
    // Send out the new buffer, note that the network stack will destroy the
    // UniqueIOBuf once it is done with it, which will in turn free the
    // underlying memory
    Send(std::move(new_buf));
    Shutdown();
  }

  void Close() override { Shutdown(); }

  void Abort() override {}
};

class StaticReceiver : public TcpHandler {
 public:
  explicit StaticReceiver(ebbrt::NetworkManager::TcpPcb pcb)
      : TcpHandler(std::move(pcb)) {}

  void Receive(std::unique_ptr<ebbrt::MutIOBuf> buf) override {
    static const char msg[] = "test\n";
    // Create a new IOBuf which refers to a static piece of memory.
    auto new_buf =
        std::unique_ptr<ebbrt::StaticIOBuf>(new ebbrt::StaticIOBuf(msg));
    Send(std::move(new_buf));
  }

  void Close() override { Shutdown(); }

  void Abort() override {}
};

ebbrt::NetworkManager::ListeningTcpPcb listening_pcb;

void AppMain() {
  // Setup the listening pcb to listen on a random port
  auto port = listening_pcb.Bind(0, [](ebbrt::NetworkManager::TcpPcb pcb) {
    // new connection callback
    ebbrt::kprintf("Connected\n");
    auto handler = new EchoReceiver(std::move(pcb));
    handler->Install();
  });

  ebbrt::kprintf("Listening on port %hu\n", port);
}
