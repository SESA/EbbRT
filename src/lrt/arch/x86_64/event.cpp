#include <new>

#include "arch/cpu.hpp"
#include "arch/x86_64/apic.hpp"
#include "arch/x86_64/idtdesc.hpp"
#include "arch/x86_64/idtr.hpp"
#include "arch/x86_64/pit.hpp"
#include "arch/x86_64/rtc.hpp"
#include "lrt/boot.hpp"
#include "lrt/event_impl.hpp"
#include "lrt/mem.hpp"
#include "lrt/arch/x86_64/exception_table.hpp"

namespace {
  ebbrt::IdtDesc idt[256];
  const int STACK_SIZE = 1 << 14; //16k
}

bool
ebbrt::lrt::event::init_arch(int num_cores)
{
  for (auto i = 0; i < 256; ++i) {
    idt[i].set(0x8, exception_table[i]);
  }

  pit::disable();
  rtc::disable();

  if (!apic::init()) {
    return false;
  }
  apic::disable_irqs();
  return true;
}

void
ebbrt::lrt::event::init_cpu_arch()
{
  Idtr idtr;
  idtr.limit = sizeof(IdtDesc) * 256 - 1;
  idtr.base = reinterpret_cast<uint64_t>(idt);
  lidt(idtr);

  Location loc = boot::smp_lock + 1;
  Location* my_loc = new (mem::malloc(sizeof(Location), loc)) Location(loc);

  wrmsr(MSR_FS_BASE, reinterpret_cast<uint64_t>(my_loc));

  if (get_location() != 0) {
    apic::Lapic::Enable();
  }

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
  boot::init_cpu();
  while(1)
    ;
}
