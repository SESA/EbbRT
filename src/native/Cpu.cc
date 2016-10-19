//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "Cpu.h"

#include "../ExplicitlyConstructed.h"
#include "PageAllocator.h"

namespace {
ebbrt::ExplicitlyConstructed<
    boost::container::static_vector<ebbrt::Cpu, ebbrt::Cpu::kMaxCpus>>
    cpus;
}

thread_local ebbrt::Cpu* ebbrt::Cpu::my_cpu_tls_;

char ebbrt::Cpu::boot_interrupt_stack_[ebbrt::pmem::kPageSize];

void ebbrt::Cpu::EarlyInit() { cpus.construct(); }

void ebbrt::Cpu::Init() {
  my_cpu_tls_ = this;
  gdt_.SetTssAddr(reinterpret_cast<uint64_t>(&atss_.tss));
  uint64_t interrupt_stack;
  if (index_ == 0) {
    interrupt_stack =
        reinterpret_cast<uint64_t>(boot_interrupt_stack_ + pmem::kPageSize);
  } else {
    auto page = page_allocator->Alloc();
    kbugon(page == Pfn::None(),
           "Unable to allocate page for interrupt stack\n");
    interrupt_stack = page.ToAddr() + pmem::kPageSize;
  }
  atss_.tss.SetIstEntry(1, interrupt_stack);
  gdt_.Load();
  idt::Load();
}

void ebbrt::Cpu::SetEventStack(uintptr_t top_of_stack) {
  atss_.tss.SetIstEntry(2, top_of_stack);
}

ebbrt::Cpu* ebbrt::Cpu::GetByIndex(size_t index) {
  if (index > cpus->size() - 1) {
    return nullptr;
  }
  return &((*cpus)[index]);
}

ebbrt::Cpu& ebbrt::Cpu::Create() {
  cpus->emplace_back(cpus->size(), 0, 0);
  return (*cpus)[cpus->size() - 1];
}

ebbrt::Cpu* ebbrt::Cpu::GetByApicId(size_t apic_id) {
  auto it = std::find_if(cpus->begin(), cpus->end(),
                         [=](const Cpu& c) { return c.apic_id() == apic_id; });
  if (it == cpus->end())
    return nullptr;
  return &(*it);
}

size_t ebbrt::Cpu::Count() { return cpus->size(); }
