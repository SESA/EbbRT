//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Pic.h>

#include <ebbrt/Cpuid.h>
#include <ebbrt/Debug.h>
#include <ebbrt/Io.h>

void ebbrt::pic::Disable() {
  if (!cpuid::features.x2apic) {
    kprintf("No support for x2apic! Aborting\n");
    kabort();
  }

  // disable pic by masking all irqs
  io::Out8(0x21, 0xff);
  io::Out8(0xa1, 0xff);
}
