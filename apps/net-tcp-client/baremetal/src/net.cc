//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <tcp_handler.hpp>

#include <ebbrt/StaticIOBuf.h>
#include <ebbrt/UniqueIOBuf.h>

class Handler : public TcpHandler {
 public:
  explicit Handler(ebbrt::NetworkManager::TcpPcb pcb)
      : TcpHandler(std::move(pcb)) {}

  // Callback invoke when connection completed
  void Connected() override {
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

  void Receive(std::unique_ptr<ebbrt::MutIOBuf> buf) override {}

  void Close() override { Shutdown(); }

  void Abort() override {}
};

ebbrt::NetworkManager::TcpPcb pcb;

void AppMain() {
  pcb.Connect(ebbrt::Ipv4Address({10, 2, 56, 1}), 49152);
  auto handler = new Handler(std::move(pcb));
  handler->Install();
}
