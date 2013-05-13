#include <cinttypes>
#include <cstdio>

#include "app/app.hpp"
#include "ebb/MemoryAllocator/MemoryAllocator.hpp"
#include "lrt/boot.hpp"
#include "lrt/console.hpp"
#include "lrt/event.hpp"
#include "misc/elf.hpp"

void
ebbrt::app::start()
{
  memory_allocator->malloc(8);
  // if (lrt::event::get_location() == 0) {
  //   const MultibootInformation* mbi = lrt::boot::multiboot_information;
  //   if (mbi->has_boot_modules && mbi->modules_count == 1) {
  //     auto mod = reinterpret_cast<MultibootModule*>(mbi->modules_address);
  //     auto config = reinterpret_cast<lrt::boot::Config*>(mod->start_address);

  //   }
  //   auto sh =  reinterpret_cast<Elf64SectionHeader*>
  //     (lrt::boot::multiboot_information->symbols[2]);
  //   unsigned i;
  //   for (i = 0;
  //        i < lrt::boot::multiboot_information->symbols[0];
  //        ++i) {
  //     if (sh[i].sh_type == Elf64SectionHeader::SHT_SYMTAB) {
  //       break;
  //     }
  //   }
  //   auto symbol_table = reinterpret_cast<Elf64SymbolTableEntry*>(sh[i].sh_addr);
  //   auto string_table = reinterpret_cast<char*>(sh[sh[i].sh_link].sh_addr);
  //   while (reinterpret_cast<uintptr_t>(symbol_table) <
  //          (sh[i].sh_addr + sh[i].sh_size)) {
  //     char out[160];
  //     sprintf(out, "%s 0x%" PRIx64 "\n",
  //             &string_table[symbol_table->st_name],
  //             symbol_table->st_value);
  //     lrt::console::write(out);
  //     symbol_table++;
  //   }
  // }
}
