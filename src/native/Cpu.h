//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_CPU_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_CPU_H_

#include "../ExplicitlyConstructed.h"
#include "../Nid.h"
#include "Apic.h"
#include "Debug.h"
#include "Idt.h"
#include "PMem.h"

namespace ebbrt {

class SegDesc {
 public:
  SegDesc() { Clear(); }
  void Clear() { raw_ = 0; }

  void Set(uint8_t type, bool longmode) {
    type_ = type;
    l_ = longmode;
    s_ = 1;
    p_ = 1;
  }
  void Set(uint64_t raw) { raw_ = raw; }

  void SetCode() { Set(8, true); }

  void SetData() { Set(0, false); }

 private:
  union {
    uint64_t raw_;
    struct {
      uint64_t limit_low_ : 16;
      uint64_t base_low_ : 24;
      uint64_t type_ : 4;
      uint64_t s_ : 1;
      uint64_t dpl_ : 2;
      uint64_t p_ : 1;
      uint64_t limit_high_ : 4;
      uint64_t avl_ : 1;
      uint64_t l_ : 1;
      uint64_t db_ : 1;
      uint64_t g_ : 1;
      uint64_t base_high_ : 8;
    };
  };
} __attribute__((packed));

static_assert(sizeof(SegDesc) == 8, "SegDesc packing issue");

class TssDesc {
 public:
  TssDesc() { Clear(); }

  void Clear() {
    raw_[0] = 0;
    raw_[1] = 0;
  }

  void Set(uint64_t addr) {
    limit_low_ = 103;
    base_low_ = addr;
    base_high_ = addr >> 24;
    p_ = true;
    type_ = 9;
  }

 private:
  union {
    uint64_t raw_[2];
    struct {
      uint64_t limit_low_ : 16;
      uint64_t base_low_ : 24;
      uint64_t type_ : 4;
      uint64_t reserved0_ : 1;
      uint64_t dpl_ : 2;
      uint64_t p_ : 1;
      uint64_t limit_high_ : 4;
      uint64_t avl_ : 1;
      uint64_t reserved1_ : 2;
      uint64_t g_ : 1;
      uint64_t base_high_ : 40;
      uint64_t reserved2_ : 32;
    } __attribute__((packed));
  };
};

static_assert(sizeof(TssDesc) == 16, "TssDesc packing issue");

class TaskStateSegment {
 public:
  void SetIstEntry(size_t entry, uint64_t addr) { ist_[entry - 1] = addr; }

 private:
  union {
    uint64_t raw_[13];
    struct {
      uint32_t reserved0_;
      uint64_t rsp_[3];
      uint64_t reserved1_;
      uint64_t ist_[7];
      uint64_t reserved2_;
      uint16_t reserved3_;
      uint16_t io_map_base_;
    } __attribute__((packed));
  };
} __attribute__((packed));

static_assert(sizeof(TaskStateSegment) == 104, "tss packing issue");

struct AlignedTss {
  uint32_t padding;
  TaskStateSegment tss;
} __attribute__((packed, aligned(8)));

class Gdt {
  SegDesc nulldesc_;
  SegDesc cs_;
  SegDesc ds_;
  SegDesc ucs_;
  SegDesc uds_;
  TssDesc tss_;

  struct DescPtr {
    DescPtr(uint16_t limit, uint64_t addr) : limit(limit), addr(addr) {}
    uint16_t limit;
    uint64_t addr;
  } __attribute__((packed));

 public:
  Gdt() {
    cs_.SetCode();
    ds_.SetData();
  }

  void Load() {
    auto gdtr = DescPtr(sizeof(Gdt) - 1, reinterpret_cast<uint64_t>(this));
    asm volatile("lgdt %[gdtr]" : : [gdtr] "m"(gdtr));
    asm volatile("ltr %w[tr]" : : [tr] "rm"(offsetof(class Gdt, tss_)));
  }

  void SetUserSegments(uint64_t cs, uint64_t ds) {
    ucs_.Set(cs);
    uds_.Set(ds);
  }

  void SetTssAddr(uint64_t tss_addr) { tss_.Set(tss_addr); }
};

class Cpu {
 public:
  static const constexpr size_t kMaxCpus = 256;

  Cpu(size_t index, uint8_t acpi_id, uint8_t apic_id)
      : index_(index), acpi_id_(acpi_id), apic_id_(apic_id) {}

  static Cpu& Create();
  static Cpu& GetMine() { return *my_cpu_tls_; }
  static Nid GetMyNode() { return GetMine().nid(); }
  static Cpu* GetByIndex(size_t index);
  static Cpu* GetByApicId(size_t apic_id);
  static size_t Count();
  static void EarlyInit();
  void Init();
  operator size_t() const { return index_; }
  uint8_t apic_id() const { return apic_id_; }
  Nid nid() const { return nid_; }
  void set_acpi_id(uint8_t id) { acpi_id_ = id; }
  void set_apic_id(uint8_t id) { apic_id_ = id; }
  void set_nid(Nid nid) { nid_ = nid; }
  Gdt* gdt() { return &gdt_; }

 private:
  void SetEventStack(uintptr_t top_of_stack);

  static char boot_interrupt_stack_[pmem::kPageSize];
  static thread_local Cpu* my_cpu_tls_;
  AlignedTss atss_;
  Gdt gdt_;
  size_t index_;
  uint8_t acpi_id_;
  uint8_t apic_id_;
  Nid nid_;

  friend class EventManager;
};

}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_CPU_H_
