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
namespace {
  const ebbrt::lrt::boot::Config* config;
  uint32_t symtab_index;
}

extern "C"
void __attribute__((noreturn))
_init_arch(ebbrt::MultibootInformation* mbi)
{
  if (mbi->has_elf_section_table) {
    for (unsigned i = 0; i < mbi->symbols[0]; ++i) {
      ebbrt::Elf64SectionHeader* esh =
        reinterpret_cast<ebbrt::Elf64SectionHeader*>(mbi->symbols[2] +
                                                   mbi->symbols[1] * i);
      ebbrt::lrt::mem::mem_start = reinterpret_cast<char*>
        (std::max(reinterpret_cast<uintptr_t>(ebbrt::lrt::mem::mem_start),
                  esh->sh_addr + esh->sh_size));
      if (esh->sh_type == ebbrt::Elf64SectionHeader::SHT_SYMTAB) {
        symtab_index = i;
      }
    }
  }
  if (mbi->has_boot_modules) {
    auto mod = reinterpret_cast<ebbrt::MultibootModule*>
      (mbi->modules_address);
    for (unsigned i = 0; i < mbi->modules_count; ++i) {
      ebbrt::lrt::mem::mem_start = reinterpret_cast<char*>
        (std::max(reinterpret_cast<uintptr_t>(ebbrt::lrt::mem::mem_start),
                  static_cast<uintptr_t>(mod->end_address)));
      char* str = mod->string;
      while (*str != '\0') {
        str++;
      }
      if (i == 0) {
        config = reinterpret_cast<ebbrt::lrt::boot::Config*>
          (mod->start_address);
      }
      mod = reinterpret_cast<ebbrt::MultibootModule*>(str);
    }
  }
  ebbrt::lrt::boot::multiboot_information = mbi;
  ebbrt::lrt::boot::init();
}

void*
ebbrt::lrt::boot::find_symbol(const char* name)
{
  auto esh = reinterpret_cast<ebbrt::Elf64SectionHeader*>
    (multiboot_information->symbols[2] +
     multiboot_information->symbols[1] * symtab_index);
  auto symtab = reinterpret_cast<ebbrt::Elf64SymbolTableEntry*>
    (esh->sh_addr);
  auto strtab_sh = reinterpret_cast<ebbrt::Elf64SectionHeader*>
    (multiboot_information->symbols[2] +
     multiboot_information->symbols[1] * esh->sh_link);
  auto strtab = reinterpret_cast<char*>(strtab_sh->sh_addr);
  while (reinterpret_cast<uintptr_t>(symtab) < (esh->sh_addr + esh->sh_size)) {
    if (std::strcmp(&strtab[symtab->st_name], name) == 0) {
      return reinterpret_cast<void*>(symtab->st_value);
    }
    symtab++;
  }
  return nullptr;
}

const ebbrt::lrt::boot::Config*
ebbrt::lrt::boot::get_config()
{
  return config;
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
