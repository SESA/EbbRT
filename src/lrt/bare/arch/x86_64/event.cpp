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
#include <new>

#include "arch/cpu.hpp"
#include "arch/x86_64/apic.hpp"
#include "arch/x86_64/idtdesc.hpp"
#include "arch/x86_64/idtr.hpp"
#include "arch/x86_64/pic.hpp"
#include "arch/x86_64/pit.hpp"
#include "arch/x86_64/rtc.hpp"
#include "ebb/EventManager/EventManager.hpp"
#include "lrt/bare/boot.hpp"
#include "lrt/event_impl.hpp"
#include "lrt/bare/mem.hpp"
#include "lrt/bare/arch/x86_64/exception_table.hpp"

namespace {
  ebbrt::IdtDesc idt[256];
  const int STACK_SIZE = 1 << 14; //16k
}

/**
 * @brief Architecture specific event initialization
 *
 * @return success/fail
 */
bool
ebbrt::lrt::event::init_arch()
{
  /* x86_64 interrupt descriptor table */
  for (auto i = 0; i < 256; ++i) {
    idt[i].set(0x8, isr_table[i]);
  }
  pic::disable();

  /* disable internal sources of interrupts */
  pit::disable();
  rtc::disable();

  if (!apic::init()) {
    return false;
  }
  /* disable external interrupts */
  apic::disable_irqs();
  return true;
}

/**
 * @brief Each core is configured in preparation to receive events. This code
 * executes sequentially across all activated cores.
 *
 * */
void
ebbrt::lrt::event::init_cpu_arch()
{
  /* set idt register */
  Idtr idtr;
  idtr.limit = sizeof(IdtDesc) * 256 - 1;
  idtr.base = reinterpret_cast<uint64_t>(idt);
  lidt(idtr);

  /* set core location */
  Location loc = boot::smp_lock + 1;
  Location* my_loc = new (mem::malloc(sizeof(Location), loc)) Location(loc);

  /* TODO: what's this guy do? */
  wrmsr(MSR_FS_BASE, reinterpret_cast<uint64_t>(my_loc));

  if (get_location() != 0) {
    apic::Lapic::Enable();
  }

  /* allocate stacks in cores address space*/
  char* stack = new (mem::memalign(16, STACK_SIZE, get_location())) char[STACK_SIZE];

  altstack[get_location()] = reinterpret_cast<uintptr_t*>
    (mem::malloc(STACK_SIZE, get_location()));

  asm volatile (
                "mov %[stack], %%rsp;"
                "incl %[smp_lock];"
                : //no output
                : [stack] "r" (&stack[STACK_SIZE]),
                  [smp_lock] "m" (boot::smp_lock)
                : "memory");
  /* when the smp_lock is incremented the next core gets woken up.  */

  /* End of our sequential execution -- here be dragons. */
  boot::init_cpu();

  /* Event loop goes here */
  while (1) {
    lrt::event::process_event();
  }
}

void
ebbrt::lrt::event::_event_interrupt(uint8_t interrupt)
{
  apic::lapic->eoi = 0;
  event_manager->HandleInterrupt(interrupt);
}
