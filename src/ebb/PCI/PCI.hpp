#ifndef EBBRT_EBB_PCI_PCI_HPP
#define EBBRT_EBB_PCI_PCI_HPP

#include <list>

#include "arch/io.hpp"
#include "ebb/ebb.hpp"

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
      uint16_t Status() const;
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

      inline void BusMaster(bool set)
      {
        Command_t c = Command();
        c.bus_master = set;
        Command(c);
      }

      // GeneralHeaderType ONLY!
      inline uint32_t BAR0() const
      {
        return read32(0x10);
      }
      uint32_t BAR1() const;
      uint32_t BAR2() const;
      uint32_t BAR3() const;
      uint32_t BAR4() const;
      uint32_t BAR5() const;
      uint32_t CardbusCISPointer() const;
      uint16_t SubsystemVendorId() const;
      inline uint16_t SubsystemId() const
      {
        return read16(0x2e);
      }
      uint32_t ExpansionROMBaseAddress() const;
      uint8_t Capabilities() const;
      uint8_t InterruptLine() const;
      uint8_t InterruptPIN() const;
      uint8_t MinGrant() const;
      uint8_t MaxLatency() const;
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

      inline void write16(uint16_t val, uint8_t offset) const
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
  extern Ebb<PCI> pci;
}
#endif
