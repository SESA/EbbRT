#include <algorithm>
#include <atomic>
#include <cstring>
#include <new>

#include "arch/cpu.hpp"
#include "arch/x86_64/multiboot.hpp"
#include "arch/x86_64/apic.hpp"
#include "lrt/boot.hpp"
#include "lrt/console.hpp"
#include "lrt/mem.hpp"
#include "lrt/arch/x86_64/smp.hpp"
#include "misc/elf.hpp"
#include "sync/compiler.hpp"

uint32_t ebbrt::lrt::boot::smp_lock;
const ebbrt::MultibootInformation* ebbrt::lrt::boot::multiboot_information;
extern "C"
void __attribute__((noreturn))
_init_arch(ebbrt::MultibootInformation* mbi)
{
  ebbrt::lrt::boot::multiboot_information = mbi;
  ebbrt::lrt::boot::init();
}

char* _smp_stack;

extern char _smp_start[];
extern char _smp_end[];
extern char _gdt_pointer[];
extern char _gdt_pointer_end[];

extern "C"
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

    while (access_once(smp_lock) != i)
      ;
  }
  access_once(smp_lock) = -1;
  _init_cpu_arch();
}
