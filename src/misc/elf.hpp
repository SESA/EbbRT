#ifndef MISC_ELF_HPP
#define MISC_ELF_HPP

#include <cstdio>

namespace ebbrt {
  class Elf64SectionHeader {
  public:
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;

    static const uint32_t SHT_SYMTAB = 2;
  };

  class Elf64SymbolTableEntry {
  public:
    uint32_t st_name;
    uint8_t st_info;
    uint8_t st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
  };
}

#endif
