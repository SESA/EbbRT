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
#ifndef EBBRT_EBB_PCI_PCI_HPP
#define EBBRT_EBB_PCI_PCI_HPP

#include <list>

#include "arch/io.hpp"
#include "ebb/ebb.hpp"
#include "lrt/bare/assert.hpp"

namespace ebbrt {
  class PCI : public EbbRep {
  public:
    static EbbRoot* ConstructRoot();
    PCI();
    class Device {
    public:
      Device(uint8_t bus, uint8_t device, uint8_t function)
        : bus_(bus), device_(device), function_(function)
      {
      }

      inline uint16_t VendorId() const
      {
        return read16(0x00);
      }

      inline uint16_t DeviceId() const
      {
        return read16(0x02);
      }
      class Command_t {
      public:
        union {
          uint16_t raw;
          struct {
            uint16_t io_space :1;
            uint16_t mem_space :1;
            uint16_t bus_master :1;
            uint16_t special_cycles :1;
            uint16_t mem_write_and_invalidate :1;
            uint16_t vga_palette_snoop :1;
            uint16_t parity_error_response :1;
            uint16_t :1;
            uint16_t serr :1;
            uint16_t fast_b2b :1;
            uint16_t interrupt :1;
            uint16_t :5;
          };
        };
      };
      inline Command_t Command() const
      {
        Command_t c;
        c.raw = read16(0x04);
        return c;
      }
      inline void Command(Command_t c)
      {
        write16(c.raw, 0x04);
      }
      class Status_t {
      public:
        union {
          uint16_t raw;
          struct {
            uint16_t :3;
            uint16_t interrupt_status :1;
            uint16_t capabilities_list :1;
            uint16_t sixtysix_mhz_capable :1;
            uint16_t :1;
            uint16_t fast_b2b_capable :1;
            uint16_t master_data_parity_error :1;
            uint16_t devsel_timing :2;
            uint16_t signaled_target_abort :1;
            uint16_t received_target_abort :1;
            uint16_t received_master_abort :1;
            uint16_t signaled_system_error :1;
            uint16_t detected_parity_error :1;
          };
        };
      };
      inline Status_t Status() const
      {
        Status_t status;
        status.raw = read16(0x06);
        return status;
      }
      uint8_t RevisionId() const;
      uint8_t ProgIF() const;
      uint8_t Subclass() const;
      static const uint8_t Subclass_PCItoPCIBridge = 0x04;
      uint8_t ClassCode() const;
      static const uint8_t ClassCode_BridgeDevice = 0x06;
      uint8_t CacheLineSize() const;
      uint8_t LatencyTimer() const;
      inline uint8_t HeaderType() const
      {
        return read8(0x0e);
      }
      uint8_t BIST() const;

      inline bool Valid() const
      {
        return VendorId() != 0xFFFF;
      }
      inline bool MultiFunction() const
      {
        return HeaderType() & (1 << 7);
      }
      inline bool GeneralHeaderType() const
      {
        return (HeaderType() & 0x7f) == 0x00;
      }

      inline bool HasCapability() const
      {
        return Status().capabilities_list;
      }

      inline void BusMaster(bool set)
      {
        Command_t c = Command();
        c.bus_master = set;
        Command(c);
      }

      // GeneralHeaderType ONLY!
      inline uint32_t BAR(uint8_t bar) const
      {
        LRT_ASSERT(bar >= 0);
        LRT_ASSERT(bar <= 6);
        return read32(0x10 + bar * 4);
      }
      uint32_t CardbusCISPointer() const;
      uint16_t SubsystemVendorId() const;
      inline uint16_t SubsystemId() const
      {
        return read16(0x2e);
      }
      uint32_t ExpansionROMBaseAddress() const;
      inline uint8_t Capabilities() const
      {
        LRT_ASSERT(HasCapability());
        return read8(0x34);
      }
      uint8_t InterruptLine() const;
      uint8_t InterruptPIN() const;
      uint8_t MinGrant() const;
      uint8_t MaxLatency() const;

      static const uint8_t CAP_MSIX = 0x11;
      inline uint8_t FindCapability(uint8_t id) const
      {
        uint8_t ptr = Capabilities() & ~0x3;
        while (ptr) {
          uint8_t type = read8(ptr);
          if (type == id) {
            break;
          }
          ptr = read8(ptr + 1) & ~0x3;
        }
        return ptr;
      }
      inline void EnableMsiX(uint8_t ptr)
      {
        uint16_t control = read16(ptr + 2);
        control |= 1 << 15; //enable
        write16(control, ptr + 2);
      }

      inline uint32_t MsiXTableOffset(uint8_t ptr) const
      {
        return read32(ptr + 4) & ~0x7;
      }

      inline uint8_t MsiXTableBIR(uint8_t ptr) const
      {
        return read32(ptr + 4) & 0x7;
      }

      inline uint32_t MsiXPBAOffset(uint8_t ptr) const
      {
        return read32(ptr + 8) & ~0x7;
      }

      inline uint8_t MsiXPBABIR(uint8_t ptr) const
      {
        return read32(ptr + 8) & 0x7;
      }

      class MSIXTableEntry {
      public:
        union {
          uint32_t raw[4];
          struct {
            uint32_t pending :1;
            uint32_t mask :1;
            uint32_t destination_mode :1;
            uint32_t redirection_hint :1;
            uint32_t :8;
            uint32_t destination :8;
            uint32_t upper :12;

            uint32_t upper_address;

            uint32_t vector :8;
            uint32_t delivery_mode :3;
            uint32_t :3;
            uint32_t level :1;
            uint32_t trigger_mode :1;
            uint32_t :16;

            uint32_t vector_control;
          };
        };
      };
    private:
      class Address {
      public:
        union {
          uint32_t raw;
          struct {
          uint32_t :2;
            uint32_t offset :6;
            uint32_t fnum :3;
            uint32_t devnum :5;
            uint32_t busnum :8;
          uint32_t :7;
            uint32_t enable :1;
          };
        };
      };

      const uint16_t PCI_ADDRESS_PORT = 0xCF8;
      const uint16_t PCI_DATA_PORT = 0xCFC;
      inline uint8_t read8(uint8_t offset) const
      {
        Address addr;
        addr.raw = 0;
        addr.enable = 1;
        addr.busnum = bus_;
        addr.devnum = device_;
        addr.fnum = function_;
        addr.offset = offset >> 2;

        out32(addr.raw, PCI_ADDRESS_PORT); // write address
        uint32_t val = in32(PCI_DATA_PORT);

        return val >> ((offset & 0x3) * 8); //offset return value
      }
      inline uint16_t read16(uint8_t offset) const
      {
        Address addr;
        addr.raw = 0;
        addr.enable = 1;
        addr.busnum = bus_;
        addr.devnum = device_;
        addr.fnum = function_;
        addr.offset = offset >> 2;

        out32(addr.raw, PCI_ADDRESS_PORT); // write address
        uint32_t val = in32(PCI_DATA_PORT);

        return val >> ((offset & 0x3) * 8); //offset return value
      }
      inline uint32_t read32(uint8_t offset) const
      {
        Address addr;
        addr.raw = 0;
        addr.enable = 1;
        addr.busnum = bus_;
        addr.devnum = device_;
        addr.fnum = function_;
        addr.offset = offset >> 2;

        out32(addr.raw, PCI_ADDRESS_PORT); // write address
        return in32(PCI_DATA_PORT);
      }

      inline void write16(uint16_t val, uint8_t offset)
      {
        Address addr;
        addr.raw = 0;
        addr.enable = 1;
        addr.busnum = bus_;
        addr.devnum = device_;
        addr.fnum = function_;
        addr.offset = offset >> 2;

        out32(addr.raw, PCI_ADDRESS_PORT); // write address
        uint32_t read_val = in32(PCI_DATA_PORT);

        read_val = (read_val & ~(0xFFFF << ((offset & 3) * 8))) |
          val << ((offset & 3) * 8);
        out32(read_val, PCI_DATA_PORT);
      }

      inline void write32(uint32_t val, uint8_t offset)
      {
        Address addr;
        addr.raw = 0;
        addr.enable = 1;
        addr.busnum = bus_;
        addr.devnum = device_;
        addr.fnum = function_;
        addr.offset = offset >> 2;

        out32(addr.raw, PCI_ADDRESS_PORT); // write address
        out32(val, PCI_DATA_PORT);
      }

      uint8_t bus_;
      uint8_t device_;
      uint8_t function_;
    };
    virtual std::list<Device>::iterator DevicesBegin();
    virtual std::list<Device>::iterator DevicesEnd();
  private:
    void EnumerateBus(uint8_t bus);

    std::list<Device> devices_;
  };
  extern EbbRef<PCI> pci;
}
#endif
