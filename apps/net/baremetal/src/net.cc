//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <ebbrt/Debug.h>
#include <ebbrt/Net.h>

void AppMain() {
  auto pcb = new ebbrt::NetworkManager::TcpPcb();
  pcb->Bind(4321);
  pcb->Listen();
  pcb->Accept([](ebbrt::NetworkManager::TcpPcb pcb) {
    auto p_pcb = new ebbrt::NetworkManager::TcpPcb(std::move(pcb));
    p_pcb->DisableNagle();
    p_pcb->Receive([](ebbrt::NetworkManager::TcpPcb& t,
                      std::unique_ptr<ebbrt::IOBuf>&& b) {});
  });
  ebbrt::kprintf("Listening\n");
}
