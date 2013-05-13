#ifndef EBBRT_ARCH_X86_64_MULTIBOOT_HPP
#define EBBRT_ARCH_X86_64_MULTIBOOT_HPP

#include <cstdint>

namespace ebbrt {
  class MultibootInformation {
  public:
    struct {
      uint32_t has_mem :1;
      uint32_t has_boot_device :1;
      uint32_t has_command_line :1;
      uint32_t has_boot_modules :1;
      uint32_t has_aout_symbol_table :1;
      uint32_t has_elf_section_table :1;
      uint32_t has_mem_map :1;
      uint32_t has_drives :1;
      uint32_t has_rom_configuration_table :1;
      uint32_t has_boot_loader_name :1;
      uint32_t has_apm_table :1;
      uint32_t has_graphics_table :1;
      uint32_t :20;
    };
    uint32_t memory_lower;
    uint32_t memory_higher;
    uint32_t boot_device;
    uint32_t command_line;
    uint32_t modules_count;
    uint32_t modules_address;
    uint32_t symbols[3];
    uint32_t memory_map_length;
    uint32_t memory_map_address;
    uint32_t drives_length;
    uint32_t drives_address;
    uint32_t configuration_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint32_t vbe_mode;
    uint32_t vbe_interface_segment;
    uint32_t vbe_interface_offset;
    uint32_t vbe_interface_length;
  };
  class MultibootMemoryRegion {
  public:
    uint32_t size;
    uint64_t base_address;
    uint64_t length;
    uint32_t type;
    static const uint32_t RAM_TYPE = 1;
  } __attribute__((packed));

  class MultibootModule {
  public:
    uint32_t start_address;
    uint32_t end_address;
    char string[0];
  };
};

#endif
