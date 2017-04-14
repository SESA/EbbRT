//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <signal.h>

#include <boost/filesystem.hpp>

#include <ebbrt/Cpu.h>
#include <ebbrt/Messenger.h>
#include <ebbrt/hosted/NodeAllocator.h>


static char* ExecName = 0;

void AppMain() {
  auto bindir = boost::filesystem::system_complete(ExecName).parent_path() /
                "/bm/iperf.elf32";
    std::cout << "EbbRT-iperf server supports a standard iperf client run with the '-C' compatibility flag" << std::endl;
    try {
      auto node = ebbrt::node_allocator->AllocateNode(bindir.string(), 2, 2, 2);
      node.NetworkId().Then(
          [](ebbrt::Future<ebbrt::Messenger::NetworkId> net_if) {
            auto net_id = net_if.Get();
            std::cout << "EbbRT-iperf server online: " << net_id.ToString() << ":5201" << std::endl;

          });
    } catch (std::runtime_error &e) {
      std::cout << e.what() << std::endl;
      exit(1);
    }
}

int main(int argc, char **argv) {
  void *status;

  ExecName = argv[0];

  pthread_t tid = ebbrt::Cpu::EarlyInit(1);

  pthread_join(tid, &status);

  return 0;
}
