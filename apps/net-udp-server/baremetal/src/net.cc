//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <ebbrt/Debug.h>
#include <ebbrt/Net.h>
#include <ebbrt/IOBufRef.h>

ebbrt::NetworkManager::UdpPcb udp_pcb;

void AppMain() {
  // have the system allocate a port for us
  auto port = udp_pcb.Bind(0);

  // Install a receive handler. This receive function could be invoked on any
  // core in the system and potentially concurrently
  udp_pcb.Receive([](ebbrt::Ipv4Address remote_address, uint16_t remote_port,
                     std::unique_ptr<ebbrt::MutIOBuf> buffer) {
    // Echo the buffer right back to the remote side. This function can be
    // invoked on any core in the system and potentially concurrently
    udp_pcb.SendTo(remote_address, remote_port, std::move(buffer));
  });

  ebbrt::kprintf("Listening on port %hu\n", port);
}
