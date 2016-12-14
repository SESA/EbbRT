//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <signal.h>

#include <boost/filesystem.hpp>

#include <ebbrt/Cpu.h>
#include <ebbrt/hosted/NodeAllocator.h>

#include "Printer.h"

static char* ExecName = 0;

void AppMain() {
  auto bindir = boost::filesystem::system_complete(ExecName).parent_path() /
                "/bm/helloworld.elf32";

  Printer::Init().Then([bindir](ebbrt::Future<void> f) {
    f.Get();
    try {
      ebbrt::node_allocator->AllocateNode(bindir.string());
    } catch (std::runtime_error& e) {
      std::cout << e.what() << std::endl;
      exit(1);
    }
  });
}

int main(int argc, char** argv) {
  void* status;

  ExecName = argv[0];

  pthread_t tid = ebbrt::Cpu::EarlyInit(1);

  pthread_join(tid, &status);

  // This is redundant I think but I think it is also likely safe
  ebbrt::Cpu::Exit(0);
  return 0;
}
