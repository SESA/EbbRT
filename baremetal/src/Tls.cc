//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Tls.h>

#include <cstring>

#include <boost/container/static_vector.hpp>

#include <ebbrt/Cpu.h>
#include <ebbrt/GeneralPurposeAllocator.h>
#include <ebbrt/Msr.h>

struct ThreadControlBlock {
  ThreadControlBlock *self;
};

extern char tcb0[];
extern char tls_start[];
extern char tls_end[];

void ebbrt::tls::Init() {
  auto tls_size = tls_end - tls_start;
  std::copy(tls_start, tls_end, tcb0);
  auto p = reinterpret_cast<ThreadControlBlock *>(tcb0 + tls_size);
  p->self = p;

  msr::Write(msr::kIa32FsBase, reinterpret_cast<uint64_t>(p));
}

namespace {
boost::container::static_vector<void *, ebbrt::Cpu::kMaxCpus> tls_ptrs;
}

void ebbrt::tls::SmpInit() {
  tls_ptrs.emplace_back(static_cast<void *>(tcb0));
  auto tls_size = align::Up(tls_end - tls_start + 8, 64);
  for (size_t i = 1; i < Cpu::Count(); ++i) {
    auto nid = Cpu::GetByIndex(i)->nid();
    auto ptr = gp_allocator->Alloc(tls_size, nid);
    kbugon(ptr == nullptr, "Failed to allocate TLS region\n");
    tls_ptrs.emplace_back(ptr);
  }
}

void ebbrt::tls::ApInit(size_t index) {
  auto tcb = static_cast<char *>(tls_ptrs[index]);
  std::copy(tls_start, tls_end, tcb);
  auto tls_size = tls_end - tls_start;
  auto p = reinterpret_cast<ThreadControlBlock *>(tcb + tls_size);
  p->self = p;

  msr::Write(msr::kIa32FsBase, reinterpret_cast<uint64_t>(p));
}
