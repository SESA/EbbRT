//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Main.h>

#include <cstdarg>
#include <cstring>
#include <stdexcept>

#include <ebbrt/Acpi.h>
#include <ebbrt/Apic.h>
#include <ebbrt/BootFdt.h>
#include <ebbrt/Console.h>
#include <ebbrt/Clock.h>
#include <ebbrt/Cpuid.h>
#include <ebbrt/Debug.h>
#include <ebbrt/E820.h>
#include <ebbrt/EarlyPageAllocator.h>
#include <ebbrt/EbbAllocator.h>
#include <ebbrt/EventManager.h>
#include <ebbrt/GeneralPurposeAllocator.h>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/MemMap.h>
#include <ebbrt/Multiboot.h>
#include <ebbrt/Net.h>
#include <ebbrt/Numa.h>
#include <ebbrt/PageAllocator.h>
#include <ebbrt/Pic.h>
#include <ebbrt/Pci.h>
#include <ebbrt/Runtime.h>
#include <ebbrt/SlabAllocator.h>
#include <ebbrt/Smp.h>
#include <ebbrt/Timer.h>
#include <ebbrt/Tls.h>
#include <ebbrt/Trans.h>
#include <ebbrt/VirtioNet.h>
#include <ebbrt/VMem.h>
#include <ebbrt/VMemAllocator.h>

namespace {
bool started_once = false;
}

// for c++ runtime
extern char __eh_frame_start[];
extern "C" void __register_frame(void* frame);
void* __dso_handle;

extern "C"
    __attribute__((noreturn)) void ebbrt::Main(multiboot::Information* mbi) {
  console::Init();

  /* If by chance we reboot back into the kernel, panic */
  kbugon(started_once, "EbbRT reboot detected... aborting!\n");
  started_once = true;

  kprintf("EbbRT Copyright 2013-2014 Boston University SESA Developers\n");

  idt::Init();
  cpuid::Init();
  tls::Init();
  clock::Init();
  pic::Disable();
  // bring up the first cpu structure early
  auto& cpu = Cpu::Create();
  cpu.set_acpi_id(0);
  cpu.set_apic_id(0);
  cpu.Init();

  e820::Init(mbi);
  e820::PrintMap();
  early_page_allocator::Init();
  multiboot::Reserve(mbi);
  boot_fdt::Init(mbi);
  extern char kend[];
  auto kend_addr = reinterpret_cast<uint64_t>(kend);
  const constexpr uint64_t initial_map_end =
      1 << 30;  // we only map 1 gb initially
  e820::ForEachUsableRegion([=](e820::Entry entry) {
    if (entry.addr() + entry.length() <= kend_addr)
      return;

    auto begin = entry.addr();
    auto length = entry.length();

    entry.TrimBelow(kend_addr);
    entry.TrimAbove(initial_map_end);
    if (entry.addr() < initial_map_end) {
      early_page_allocator::FreeMemoryRange(entry.addr(), entry.length());
    }
    vmem::EarlyMapMemory(begin, length);
  });

  vmem::EnableRuntimePageTable();

  e820::ForEachUsableRegion([=](e820::Entry entry) {
    if (entry.addr() + entry.length() < initial_map_end)
      return;

    entry.TrimBelow(initial_map_end);
    early_page_allocator::FreeMemoryRange(entry.addr(), entry.length());
  });

  numa::EarlyInit();
  acpi::BootInit();
  numa::Init();
  mem_map::Init();
  trans::Init();
  PageAllocator::Init();
  slab::Init();
  GeneralPurposeAllocatorType::Init();
  LocalIdMap::Init();
  EbbAllocator::Init();
  VMemAllocator::Init();
  EventManager::Init();

  event_manager->SpawnLocal([]() {
    // Enable exceptions
    __register_frame(__eh_frame_start);
    apic::Init();
    Timer::Init();
    smp::Init();
    NetworkManager::Init();
    pci::Init();
    pci::RegisterProbe(VirtioNetDriver::Probe);
    pci::LoadDrivers();
    network_manager->AcquireIPAddress();
    GlobalIdMap::Init();
    runtime::Init();
    kprintf("System initialization complete\n");
  });

  event_manager->StartProcessingEvents();
}
