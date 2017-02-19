//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <chrono>
#include <signal.h>

#include <boost/filesystem.hpp>

#include <ebbrt/Cpu.h>
#include <ebbrt/hosted/PoolAllocator.h>

#include "Printer.h"

static char* ExecName = 0;
static unsigned int NumNodes = 2;

void AppMain() {
  auto bindir = boost::filesystem::system_complete(ExecName).parent_path() /
    "/bm/helloworld.elf32";
  unsigned int num_nodes = NumNodes;

  struct timeval START_TIME;
  gettimeofday(&START_TIME, NULL);
  Printer::Init().Then([bindir, num_nodes](ebbrt::Future<void> f) {
    f.Get();
    try {
      ebbrt::pool_allocator->AllocatePool(bindir.string(), num_nodes);
    } catch (std::runtime_error& e) {
      std::cout << e.what() << std::endl;
      exit(1);
    }
  });


  ebbrt::pool_allocator->waitPool().Then([START_TIME](ebbrt::Future<void> f) {
    f.Get();
    struct timeval END_TIME;
    gettimeofday(&END_TIME, NULL);
    std::printf("ALLOCATION TIME: %lf seconds\n",
      (END_TIME.tv_sec - START_TIME.tv_sec) +
      ((END_TIME.tv_usec - START_TIME.tv_usec) / 1000000.0));
  });
}

int main(int argc, char** argv) {
  void* status;

  ExecName = argv[0];
  if (argc > 1) {
    NumNodes = std::atoi(argv[1]);
  }

  pthread_t tid = ebbrt::Cpu::EarlyInit((size_t) NumNodes);
  pthread_join(tid, &status);

  // This is redundant I think but I think it is also likely safe
  ebbrt::Cpu::Exit(0);
  return 0;
}
