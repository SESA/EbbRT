/*
  EbbRT: Distributed, Elastic, Runtime
  Copyright (C) 2013 SESA Group, Boston University

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <algorithm>
#include <atomic>
#include <cstring>
#include <string>
#include <new>

#include "arch/cpu.hpp"
#include "arch/x86_64/multiboot.hpp"
#include "arch/x86_64/apic.hpp"
#include "lib/fdt/libfdt.h"
#include "lrt/bare/assert.hpp"
#include "lrt/bare/boot.hpp"
#include "lrt/bare/console.hpp"
#include "lrt/event.hpp"
#include "lrt/bare/mem.hpp"
#include "lrt/bare/arch/x86_64/smp.hpp"
#include "misc/elf.hpp"
#include "sync/compiler.hpp"

uint32_t ebbrt::lrt::boot::smp_lock;
const ebbrt::MultibootInformation* ebbrt::lrt::boot::multiboot_information;
void* ebbrt::lrt::boot::fdt;
extern "C"
/**
 * @brief Initial architecture initialization. This acts as C++ entry upcalled
 * from assembly. For this reason, the function is not available elsewhere in
 * the system.
 *
 * @param mbi Multiboot information structure
 */
void __attribute__((noreturn))
_init_arch(ebbrt::MultibootInformation* mbi)
{
  ebbrt::lrt::boot::multiboot_information = mbi;

  // resolve address of FDT through Multiboot interface
  auto addr_val = static_cast<uintptr_t>(ebbrt::lrt::boot::multiboot_information->modules_address);
  auto addr = reinterpret_cast<uint32_t*>(addr_val);
  uint32_t mod_val = *addr;
  ebbrt::lrt::boot::fdt = reinterpret_cast<void *>(mod_val);
  LRT_ASSERT(( fdt_check_header(ebbrt::lrt::boot::fdt) == 0 ));
  ebbrt::lrt::boot::init();
}

char* _smp_stack;

extern char _smp_start[];
extern char _smp_end[];
extern char _gdt_pointer[];
extern char _gdt_pointer_end[];

extern "C"
/**
 * @brief C++ entry for secondary cores, upcalled from asm
 *
 * @param
 */
void __attribute__((noreturn))
_init_cpu_arch()
{
  ebbrt::lrt::event::init_cpu();
}

void
ebbrt::lrt::boot::init_smp(unsigned num_cores)
{
  _smp_stack = new (mem::memalign(16, SMP_STACK_SIZE, 0)) char[SMP_STACK_SIZE];

  std::copy(_smp_start, _smp_end, reinterpret_cast<char*>(SMP_START_ADDRESS));
  std::copy(_gdt_pointer, _gdt_pointer_end,
            reinterpret_cast<char*>(SMP_START_ADDRESS) + (_smp_end - _smp_start));

  apic::Lapic::Enable();
  for (unsigned i = 1; i < num_cores; ++i) {
    /* Wake each core (1 .. num_cores) sequentually by sending high/low apic ipis and
     * spinning untill the core has initilized. */
    apic::Lapic::IcrLow icr_low;
    icr_low.raw_ = 0;
    icr_low.delivery_mode_ = apic::Lapic::IcrLow::DELIVERY_INIT;
    icr_low.level_ = 1;
    apic::Lapic::IcrHigh icr_high;
    icr_high.raw_ = 0;
    //FIXME: translate to apic id
    icr_high.destination_ = i;

    apic::lapic->SendIpi(icr_low, icr_high);

    uint64_t time = rdtsc();
    while ((rdtsc() - time) < 1000000)
      ;

    icr_low.vector_ = SMP_START_ADDRESS >> 12;
    icr_low.delivery_mode_ = apic::Lapic::IcrLow::DELIVERY_STARTUP;

    apic::lapic->SendIpi(icr_low, icr_high);

    /* core i is responsible for increasing this count */
    while (access_once(smp_lock) != i)
      ;
  }
  /* at this point, secondary cores are initialized and running */
  access_once(smp_lock) = -1;
  _init_cpu_arch();
}

