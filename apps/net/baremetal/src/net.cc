//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <ebbrt/Debug.h>
#include <ebbrt/Net.h>
#include <ebbrt/StaticIOBuf.h>

ebbrt::NetworkManager::TcpPcb pcb;

void AppMain() {
  pcb.Bind(54321);
  // pcb.Bind(12345);
  // auto buf =
  //     std::unique_ptr<ebbrt::StaticIOBuf>(new ebbrt::StaticIOBuf("test"));
  // pcb.SendTo({{10, 1, 23, 1}}, 54321, std::move(buf));
}
