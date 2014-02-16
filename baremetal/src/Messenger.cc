//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Messenger.h>

#include <ebbrt/Debug.h>

uint16_t ebbrt::Messenger::port_;

ebbrt::Messenger::Messenger() {}

void ebbrt::Messenger::StartListening(uint16_t port) {
  port_ = port;
  tcp_.Bind(port);
  tcp_.Listen();
  tcp_.Accept([this](NetworkManager::TcpPcb pcb) {
    kprintf("New Messenger Connection\n");
  });
}

void ebbrt::Messenger::Receive(Buffer b) {
  kbugon(b.length() > 1, "handle multiple length buffer\n");
  kbugon(b.size() < sizeof(Header), "buffer too small\n");
  auto p = b.GetDataPointer();
  auto length = p.Get<uint64_t>();
  kprintf("Received %d\n", length);
  kbugon(b.size() < (sizeof(Header) + length),
         "Did not receive complete message\n");
}
