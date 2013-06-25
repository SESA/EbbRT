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
#ifndef EBBRT_ARCH_X86_64_APIC_HPP
#define EBBRT_ARCH_X86_64_APIC_HPP

#include <atomic>
#include <cstdint>

namespace ebbrt {
  namespace apic {
    bool init();
    void disable_irqs();
    void enable_lapic();

    class IoApic {
    public:
      uint32_t Read(int index);
      void Write(int index, uint32_t val);
      int Id();
      int MaxRedEntry();
      int ApicVersion();
      class IoApicVersionRegister {
      public:
        IoApicVersionRegister(uint32_t i) : raw(i) {
        }
        union {
          uint32_t raw;
          struct {
            uint32_t version :8;
            uint32_t :8;
            uint32_t max_redir_entry :8;
            uint32_t :8;
          } __attribute__((packed));
        };
      };
      IoApicVersionRegister ReadVersion();
      class IoApicRedEntry {
      public:
        IoApicRedEntry();
        void Disable();
        operator uint64_t() {
          return raw_;
        }
      private:
        union {
          uint64_t raw_;
          struct {
            uint64_t intvec_ :8;
            uint64_t delmod_ :3;
            uint64_t destmod_ :1;
            uint64_t delivs_ :1;
            uint64_t intpol_ :1;
            uint64_t remote_irr_ :1;
            uint64_t trigger_mode_ :1;
            uint64_t interrupt_mask_ :1;
            uint64_t :39;
            uint64_t destination_ :8;
          };
        };
      } __attribute__((packed));
      IoApicRedEntry readEntry(int i);
      void WriteEntry(int i, IoApicRedEntry entry);
      static const int IO_APIC_ID = 0;
      static const int IO_APIC_VER = 1;
      static const int IO_APIC_ARB = 2;
      static const int IO_APIC_REDTBL_START = 0x10;
      static const int IO_APIC_REDTBL_END = 0x3f;
    private:
      uint32_t io_reg_sel_;
      std::atomic<uint32_t> io_win_ __attribute__((aligned(16)));
    };
    extern IoApic** io_apics;
    extern int num_io_apics;

    class Lapic {
    public:
      static void Enable();
      void SwEnable();
      class IcrLow {
      public:
        union {
          uint32_t raw_;
          struct {
            uint32_t vector_ :8;
            uint32_t delivery_mode_ :3;
            uint32_t destination_mode_ :1;
            uint32_t delivery_status_ :1;
            uint32_t :1;
            uint32_t level_ :1;
            uint32_t trigger_mode_ :1;
            uint32_t :2;
            uint32_t destination_shorthand_ :2;
            uint32_t :12;
          };
        };
        static const uint32_t DELIVERY_INIT = 5;
        static const uint32_t DELIVERY_STARTUP = 6;
      };
      class IcrHigh {
      public:
        union {
          uint32_t raw_;
          struct {
            uint32_t :24;
            uint32_t destination_ :8;
          };
        };
      };
    private:
      class Sivr {
      public:
        union {
          uint32_t raw_;
          struct {
            uint32_t vector_ :8;
            uint32_t sw_enable_ :1;
            uint32_t focus_check_ :1;
            uint32_t :2;
            uint32_t eoi_suppression_ :1;
            uint32_t :19;
          };
        };
      };
      union {
        uint32_t raw[256];
        struct {
          volatile uint32_t reserved0_ __attribute__((aligned(16)));
          volatile uint32_t reserved1_ __attribute__((aligned(16)));
          volatile uint32_t lir_ __attribute__((aligned(16)));
          volatile uint32_t lvr_ __attribute__((aligned(16)));
          volatile uint32_t reserved2_ __attribute__((aligned(16)));
          volatile uint32_t reserved3_ __attribute__((aligned(16)));
          volatile uint32_t reserved4_ __attribute__((aligned(16)));
          volatile uint32_t reserved5_ __attribute__((aligned(16)));
          volatile uint32_t tpr_ __attribute__((aligned(16)));
          volatile uint32_t apr_ __attribute__((aligned(16)));
          volatile uint32_t ppr_ __attribute__((aligned(16)));
          volatile uint32_t eoi __attribute__((aligned(16)));
          volatile uint32_t rrd_ __attribute__((aligned(16)));
          volatile uint32_t ldr_ __attribute__((aligned(16)));
          volatile uint32_t dfr_ __attribute__((aligned(16)));
          volatile Sivr sivr_ __attribute__((aligned(16)));
          volatile uint32_t isr_31_0_ __attribute__((aligned(16)));
          volatile uint32_t isr_63_32_ __attribute__((aligned(16)));
          volatile uint32_t isr_95_64_ __attribute__((aligned(16)));
          volatile uint32_t isr_127_96_ __attribute__((aligned(16)));
          volatile uint32_t isr_159_128_ __attribute__((aligned(16)));
          volatile uint32_t isr_191_160_ __attribute__((aligned(16)));
          volatile uint32_t isr_223_192_ __attribute__((aligned(16)));
          volatile uint32_t isr_255_224_ __attribute__((aligned(16)));
          volatile uint32_t tmr_31_0_ __attribute__((aligned(16)));
          volatile uint32_t tmr_63_32_ __attribute__((aligned(16)));
          volatile uint32_t tmr_95_64_ __attribute__((aligned(16)));
          volatile uint32_t tmr_127_96_ __attribute__((aligned(16)));
          volatile uint32_t tmr_159_128_ __attribute__((aligned(16)));
          volatile uint32_t tmr_191_160_ __attribute__((aligned(16)));
          volatile uint32_t tmr_223_192_ __attribute__((aligned(16)));
          volatile uint32_t tmr_255_224_ __attribute__((aligned(16)));
          volatile uint32_t irr_31_0_ __attribute__((aligned(16)));
          volatile uint32_t irr_63_32_ __attribute__((aligned(16)));
          volatile uint32_t irr_95_64_ __attribute__((aligned(16)));
          volatile uint32_t irr_127_96_ __attribute__((aligned(16)));
          volatile uint32_t irr_159_128_ __attribute__((aligned(16)));
          volatile uint32_t irr_191_160_ __attribute__((aligned(16)));
          volatile uint32_t irr_223_192_ __attribute__((aligned(16)));
          volatile uint32_t irr_255_224_ __attribute__((aligned(16)));
          volatile uint32_t esr_ __attribute__((aligned(16)));
          volatile uint32_t reserved6_ __attribute__((aligned(16)));
          volatile uint32_t reserved7_ __attribute__((aligned(16)));
          volatile uint32_t reserved8_ __attribute__((aligned(16)));
          volatile uint32_t reserved9_ __attribute__((aligned(16)));
          volatile uint32_t reserved11_ __attribute__((aligned(16)));
          volatile uint32_t reserved12_ __attribute__((aligned(16)));
          volatile uint32_t lvt_cmci_ __attribute__((aligned(16)));
          volatile IcrLow lil_ __attribute__((aligned(16)));
          volatile IcrHigh lih_ __attribute__((aligned(16)));
          volatile uint32_t lvt_timer_ __attribute__((aligned(16)));
          volatile uint32_t lvt_thermal_ __attribute__((aligned(16)));
          volatile uint32_t lvt_pmc_ __attribute__((aligned(16)));
          volatile uint32_t lvt_lint0_ __attribute__((aligned(16)));
          volatile uint32_t lvt_lint1_ __attribute__((aligned(16)));
          volatile uint32_t lvt_error_ __attribute__((aligned(16)));
          volatile uint32_t init_count_ __attribute__((aligned(16)));
          volatile uint32_t current_count_ __attribute__((aligned(16)));
          volatile uint32_t reserved13_ __attribute__((aligned(16)));
          volatile uint32_t reserved14_ __attribute__((aligned(16)));
          volatile uint32_t reserved15_ __attribute__((aligned(16)));
          volatile uint32_t reserved16_ __attribute__((aligned(16)));
          volatile uint32_t dcr_ __attribute__((aligned(16)));
          volatile uint32_t reserved17_ __attribute__((aligned(16)));
        };
      };
    public:
      void SendIpi(IcrLow icr_low, IcrHigh icr_high);
    };
    Lapic* const lapic = reinterpret_cast<Lapic*>(0xFEE00000);
  };
}

#endif
