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
#ifndef EBBRT_ARCH_X86_64_HPP
#define EBBRT_ARCH_X86_64_HPP

#include <cstdint>

namespace ebbrt {
  const int PAGE_SIZE = 4096;
  class Pml4Entry {
  public:
    union {
      uint64_t raw;
      struct {
        uint64_t present :1;
        uint64_t rw :1;
        uint64_t privilege :1;
        uint64_t write_through :1;
        uint64_t cache_disable :1;
        uint64_t accessed :1;
        uint64_t :6;
        uint64_t base :40;
        uint64_t :11;
        uint64_t nx :1;
      } __attribute__((packed));
    };
  };
  static_assert(sizeof(Pml4Entry) == 8, "Pml4Entry packing issue");

  class PdptEntry {
  public:
    union {
      uint64_t raw;
      struct {
        uint64_t present :1;
        uint64_t rw :1;
        uint64_t privilege :1;
        uint64_t write_through :1;
        uint64_t cache_disable :1;
        uint64_t accessed :1;
        uint64_t :1;
        uint64_t ps :1; // must be unset for standard entry
        uint64_t :4;
        uint64_t base :40;
        uint64_t :11;
        uint64_t nx :1;
      } __attribute__((packed));
    };
  };
  static_assert(sizeof(PdptEntry) == 8, "PdptEntry packing issue");

  class Pd2mEntry {
  public:
    union {
      uint64_t raw_;
      struct {
        uint64_t present :1;
        uint64_t rw :1;
        uint64_t privilege :1;
        uint64_t write_through :1;
        uint64_t cache_disable :1;
        uint64_t accessed :1;
        uint64_t dirty :1;
        uint64_t ps :1; // must be set for 2m entry
        uint64_t global :1;
        uint64_t :3;
        uint64_t pat :1;
        uint64_t :8;
        uint64_t base :31;
        uint64_t :11;
        uint64_t nx :1;
      } __attribute__((packed));
    };
  };
  static_assert(sizeof(Pd2mEntry) == 8, "Pd2mEntry packing issue");

  class PdEntry {
  public:
    union {
      uint64_t raw;
      struct {
        uint64_t present :1;
        uint64_t rw :1;
        uint64_t privilege :1;
        uint64_t write_through :1;
        uint64_t cache_disable :1;
        uint64_t accessed :1;
        uint64_t :1;
        uint64_t ps :1; // must be unset for standard entry
        uint64_t :4;
        uint64_t base :40;
        uint64_t :11;
        uint64_t nx :1;
      } __attribute__((packed));
    };
  };
  static_assert(sizeof(PdEntry) == 8, "PdEntry packing issue");

  class PtabEntry {
  public:
    union {
      uint64_t raw;
      struct {
        uint64_t present :1;
        uint64_t rw :1;
        uint64_t privilege :1;
        uint64_t write_through :1;
        uint64_t cache_disable :1;
        uint64_t accessed :1;
        uint64_t dirty :1;
        uint64_t pat :1; // must be set for 2m entry
        uint64_t global :1;
        uint64_t :3;
        uint64_t base :40;
        uint64_t :11;
        uint64_t nx :1;
      } __attribute__((packed));
    };
  };
  static_assert(sizeof(PtabEntry) == 8, "PtabEntry packing issue");

  inline Pml4Entry*
  getPml4() {
    Pml4Entry* ret;
    asm volatile (
                  "mov %%cr3, %[pml4]"
                  : [pml4] "=r" (ret)
                  );
    return ret;
  }

  inline void
  set_pml4(Pml4Entry* pml4)
  {
    asm volatile (
                  "mov %[pml4], %%cr3"
                  :
                  : [pml4] "r" (pml4)
                  );
  }

  const int PML4_INDEX_SHIFT = 39;
  const int PML4_INDEX_BITS = 9;

  const int PDPT_INDEX_SHIFT = 30;
  const int PDPT_INDEX_BITS = 9;

  const int PDIR_INDEX_SHIFT = 21;
  const int PDIR_INDEX_BITS = 9;

  const int PTAB_INDEX_SHIFT = 12;
  const int PTAB_INDEX_BITS = 9;

  const int PML4_SIZE = 4096;
  const int PDPT_SIZE = 4096;
  const int PDIR_SIZE = 4096;
  const int PTAB_SIZE = 4096;

  const int PML4_ALIGN = 4096;
  const int PDPT_ALIGN = 4096;
  const int PDIR_ALIGN = 4096;
  const int PTAB_ALIGN = 4096;

  const int PML4_NUM_ENTS = PML4_SIZE / sizeof(Pml4Entry);
  const int PDPT_NUM_ENTS = PDPT_SIZE / sizeof(PdptEntry);
  const int PDIR_NUM_ENTS = PDIR_SIZE / sizeof(PdEntry);
  const int PTAB_NUM_ENTS = PTAB_SIZE / sizeof(PtabEntry);
  inline int pml4_index(uintptr_t addr) {
    return (addr >> PML4_INDEX_SHIFT) &
      ((1 << PML4_INDEX_BITS) - 1);
  }
  inline int pdpt_index(uintptr_t addr) {
    return (addr >> PDPT_INDEX_SHIFT) &
      ((1 << PDPT_INDEX_BITS) - 1);
  }
  inline int pdir_index(uintptr_t addr) {
    return (addr >> PDIR_INDEX_SHIFT) &
      ((1 << PDIR_INDEX_BITS) - 1);
  }
  inline int ptab_index(uintptr_t addr) {
    return (addr >> PTAB_INDEX_SHIFT) &
      ((1 << PTAB_INDEX_BITS) - 1);
  }
}

#endif
