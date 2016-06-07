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
#ifdef __EBBRT_ENABLE_FDT__
#include <ebbrt/BootFdt.h>
#endif
#include <ebbrt/Clock.h>
#include <ebbrt/Console.h>
#include <ebbrt/Cpuid.h>
#include <ebbrt/Debug.h>
#include <ebbrt/E820.h>
#include <ebbrt/EarlyPageAllocator.h>
#include <ebbrt/EbbAllocator.h>
#include <ebbrt/EventManager.h>
#include <ebbrt/GeneralPurposeAllocator.h>
#ifdef __EBBRT_ENABLE_DISTRIBUTED_RUNTIME__
#include <ebbrt/GlobalIdMap.h>
#endif
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/MemMap.h>
#include <ebbrt/Messenger.h>
#include <ebbrt/Multiboot.h>
#ifdef __EBBRT_ENABLE_NETWORKING__
#include <ebbrt/Net.h>
#endif
#include <ebbrt/Numa.h>
#include <ebbrt/PageAllocator.h>
#include <ebbrt/Pci.h>
#include <ebbrt/Pic.h>
#include <ebbrt/Random.h>
#ifdef __EBBRT_ENABLE_DISTRIBUTED_RUNTIME__
#include <ebbrt/Runtime.h>
#endif
#include <ebbrt/SlabAllocator.h>
#include <ebbrt/Smp.h>
#include <ebbrt/Timer.h>
#include <ebbrt/Tls.h>
#ifdef __EBBRT_ENABLE_TRACE__
#include <ebbrt/Trace.h>
#endif
#include <ebbrt/Trans.h>
#include <ebbrt/VMem.h>
#include <ebbrt/VMemAllocator.h>
#include <ebbrt/VirtioNet.h>

namespace {
bool started_once = false;
}

extern void AppMain() __attribute__((weak));

// for c++ runtime
extern char __eh_frame_start[];
extern "C" void __register_frame(void* frame);
void* __dso_handle;
extern void (*start_ctors[])();
extern void (*end_ctors[])();

extern "C" __attribute__((noreturn)) void
ebbrt::Main(multiboot::Information* mbi) {
  console::Init();

#ifdef __EBBRT_ENABLE_TRACE__
  trace::Init();
#endif

  /* If by chance we reboot back into the kernel, panic */
  kbugon(started_once, "EbbRT reboot detected... aborting!\n");
  started_once = true;

  kprintf("EbbRT Copyright 2013-2015 Boston University SESA Developers\n");

  idt::Init();
  cpuid::Init();
  tls::Init();
  clock::Init();
  random::Init();
  pic::Disable();
  Cpu::EarlyInit();
  // bring up the first cpu structure early
  auto& cpu = Cpu::Create();
  cpu.set_acpi_id(0);
  cpu.set_apic_id(0);
  cpu.Init();

  e820::Init(mbi);
  e820::PrintMap();

  early_page_allocator::Init();
  multiboot::Reserve(mbi);
#ifdef __EBBRT_ENABLE_FDT__
  boot_fdt::Init(mbi);
#endif
  vmem::Init();
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

  event_manager->SpawnLocal(
      [=]() {
        /// Enable exceptions
        __register_frame(__eh_frame_start);
        apic::Init();
        apic::PVEoiInit(0);
        Timer::Init();
        smp::Init();
        event_manager->ReceiveToken();
#ifdef __EBBRT_ENABLE_NETWORKING__
        NetworkManager::Init();
        pci::Init();
        pci::RegisterProbe(VirtioNetDriver::Probe);
        pci::LoadDrivers();
        network_manager->StartDhcp().Then([](Future<void> fut) {
          fut.Get();
// Dhcp completed
#ifdef __EBBRT_ENABLE_DISTRIBUTED_RUNTIME__
          Messenger::Init();
          runtime::Init();
#endif
#endif
          // run global ctors
          for (unsigned i = 0; i < (end_ctors - start_ctors); ++i) {
            start_ctors[i]();
          }
          kprintf("System initialization complete\n");
          if (AppMain) {
            event_manager->SpawnLocal([=]() { AppMain(); });
          }
#ifdef __EBBRT_ENABLE_NETWORKING__
        });
#endif
      },
      /* force_async = */ true);
  event_manager->StartProcessingEvents();
}
