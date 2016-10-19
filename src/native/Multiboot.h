//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_MULTIBOOT_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_MULTIBOOT_H_
#pragma once

#include <cstddef>
#include <cstdint>

namespace ebbrt {
namespace multiboot {
struct Information {
  struct {
    uint32_t has_mem_ : 1;
    uint32_t has_boot_device_ : 1;
    uint32_t has_command_line_ : 1;
    uint32_t has_boot_modules_ : 1;
    uint32_t has_aout_symbol_table_ : 1;
    uint32_t has_elf_section_table_ : 1;
    uint32_t has_mem_map_ : 1;
    uint32_t has_drives_ : 1;
    uint32_t has_rom_configuration_table_ : 1;
    uint32_t has_boot_loader_name_ : 1;
    uint32_t has_apm_table_ : 1;
    uint32_t has_graphics_table_ : 1;
    uint32_t reserved_ : 20;
  };
  uint32_t memory_lower_;
  uint32_t memory_higher_;
  uint32_t boot_device_;
  uint32_t command_line_;
  uint32_t modules_count_;
  uint32_t modules_address_;
  uint32_t symbols_[4];
  uint32_t memory_map_length_;
  uint32_t memory_map_address_;
  uint32_t drives_length_;
  uint32_t drives_address_;
  uint32_t configuration_table_;
  uint32_t boot_loader_name_;
  uint32_t apm_table_;
  uint32_t vbe_control_info_;
  uint32_t vbe_mode_info_;
  uint32_t vbe_mode_;
  uint32_t vbe_interface_segment_;
  uint32_t vbe_interface_offset_;
  uint32_t vbe_interface_length_;
};

struct MemoryRegion {
  static const constexpr uint32_t kRamType = 1;
  static const constexpr uint32_t kReservedUnusable = 2;
  static const constexpr uint32_t kAcpiReclaimable = 3;
  static const constexpr uint32_t kAcpiNvs = 4;
  static const constexpr uint32_t kBadMemory = 5;

  template <typename F>
  static void ForEachRegion(const void* buffer, uint32_t size, F f) {
    auto end_addr = static_cast<const char*>(buffer) + size;
    auto p = static_cast<const char*>(buffer);
    while (p < end_addr) {
      auto region = reinterpret_cast<const MemoryRegion*>(p);
      f(*region);
      p += region->size_ + 4;
    }
  }

  uint32_t size_;
  uint64_t base_address_;
  uint64_t length_;
  uint32_t type_;
} __attribute__((packed));

struct Module {
  uint32_t start;
  uint32_t end;
  uint32_t string;  // NOLINT
  uint32_t reserved;
};

const char* CmdLine();
void Reserve(Information* mbi);

extern uintptr_t cmdline_addr_;
extern uintptr_t cmdline_len_;
extern struct Information* info;

}  // namespace multiboot
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_MULTIBOOT_H_
