//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Smp.h>

#include <algorithm>

#include <ebbrt/Clock.h>
#include <ebbrt/Cpu.h>
#include <ebbrt/EventManager.h>
#include <ebbrt/PageAllocator.h>
#include <ebbrt/SpinBarrier.h>
#include <ebbrt/Tls.h>
#include <ebbrt/VMem.h>

extern char smpboot[];
extern char smpboot_end[];
extern char* smp_stack_free;

namespace {
ebbrt::ExplicitlyConstructed<ebbrt::SpinBarrier> smp_barrier;
}

void ebbrt::smp::Init() {
  smp_barrier.construct(Cpu::Count());
  tls::SmpInit();
  char* stack_list = 0;
  auto num_aps = Cpu::Count() - 1;
  for (size_t i = 0; i < num_aps; i++) {
    auto pfn = page_allocator->Alloc();
    kbugon(pfn == Pfn::None(), "Failed to allocate smp stack!\n");
    auto addr = pfn.ToAddr();
    kbugon(addr >= 1 << 30, "Stack will not be accessible by APs!\n");
    *reinterpret_cast<char**>(addr) = stack_list;
    stack_list = reinterpret_cast<char*>(addr);
  }
  vmem::MapMemory(Pfn::Down(SMP_START_ADDRESS), Pfn::Down(SMP_START_ADDRESS));
  smp_stack_free = stack_list;
  std::copy(smpboot, smpboot_end, reinterpret_cast<char*>(SMP_START_ADDRESS));
  // TODO(dschatz): unmap memory

  for (size_t i = 1; i < Cpu::Count(); ++i) {
    auto apic_id = Cpu::GetByIndex(i)->apic_id();
    apic::Ipi(apic_id, 0, 1, apic::kDeliveryInit);
    // FIXME: spin for a bit?
    apic::Ipi(apic_id, SMP_START_ADDRESS >> 12, 1, apic::kDeliveryStartup);
  }
  smp_barrier->Wait();
}

extern "C" __attribute__((noreturn)) void ebbrt::smp::SmpMain() {
  apic::Init();
  auto id = apic::GetId();
  auto cpu = Cpu::GetByApicId(id);
  kassert(cpu != nullptr);
  size_t cpu_index = *cpu;
  kprintf("Core %llu online\n", cpu_index);
  vmem::ApInit(cpu_index);
  trans::ApInit(cpu_index);
  tls::ApInit(cpu_index);
  clock::ApInit();

  cpu->Init();

  event_manager->SpawnLocal([]() { smp_barrier->Wait(); });
  event_manager->StartProcessingEvents();
}
