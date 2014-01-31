//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/BootFdt.h>

extern "C" {
#include <libfdt.h>
}

#include <ebbrt/Debug.h>
#include <ebbrt/ExplicitlyConstructed.h>

namespace {
ebbrt::ExplicitlyConstructed<ebbrt::FdtReader> the_fdt;
}

void ebbrt::boot_fdt::Init(ebbrt::multiboot::Information* mbi) {
  if (!mbi->has_boot_modules_ || mbi->modules_count_ < 1) {
    kprintf("No FDT found\n");
    return;
  }

  auto mods = reinterpret_cast<multiboot::Module*>(
      static_cast<uintptr_t>(mbi->modules_address_));

  // We assume the first module is the FDT
  uintptr_t fdt_addr = mods[0].start;
  kbugon(fdt_addr < 1 << 20,
         "fdt is located below 1MB and therefore not mapped\n");
  auto fdt_ptr = reinterpret_cast<const void*>(fdt_addr);

  auto err = fdt_check_header(fdt_ptr);
  if (err != 0) {
    kabort("fdt_check_header failed");
  } else {
    kprintf("Fdt located\n");
    the_fdt.construct(fdt_ptr);
  }
}

ebbrt::FdtReader ebbrt::boot_fdt::Get() { return *the_fdt; }
