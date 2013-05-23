#ifndef EBBRT_DEVICE_PCI_HPP
#define EBBRT_DEVICE_PCI_HPP

#include <cstdint>
#include <list>
#include <ostream>

namespace ebbrt {
  namespace pci {

    class Device {
    public:
      Device(uint8_t bus, uint8_t device, uint8_t function);
      uint16_t VendorId() const;
      uint16_t DeviceId() const;
      class Command_t {
      public:
        Command_t(uint16_t val) : raw(val) {}
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
      Command_t Command() const;
      void Command(Command_t c) const;
      uint16_t Status() const;
      uint8_t RevisionId() const;
      uint8_t ProgIF() const;
      uint8_t Subclass() const;
      static const uint8_t Subclass_PCItoPCIBridge = 0x04;
      uint8_t ClassCode() const;
      static const uint8_t ClassCode_BridgeDevice = 0x06;
      uint8_t CacheLineSize() const;
      uint8_t LatencyTimer() const;
      uint8_t HeaderType() const;
      uint8_t BIST() const;

      bool Valid() const;
      bool MultiFunction() const;
      bool GeneralHeaderType() const;

      void BusMaster(bool set) const;

      // GeneralHeaderType ONLY!
      uint32_t BAR0() const;
      uint32_t BAR1() const;
      uint32_t BAR2() const;
      uint32_t BAR3() const;
      uint32_t BAR4() const;
      uint32_t BAR5() const;
      uint32_t CardbusCISPointer() const;
      uint16_t SubsystemVendorId() const;
      uint16_t SubsystemId() const;
      uint32_t ExpansionROMBaseAddress() const;
      uint8_t Capabilities() const;
      uint8_t InterruptLine() const;
      uint8_t InterruptPIN() const;
      uint8_t MinGrant() const;
      uint8_t MaxLatency() const;
    private:
      uint8_t read8(uint8_t offset) const;
      uint16_t read16(uint8_t offset) const;
      uint32_t read32(uint8_t offset) const;

      void write16(uint16_t val, uint8_t offset) const;
      friend std::ostream& operator<<(std::ostream& os, const Device& d);

      uint8_t bus_;
      uint8_t device_;
      uint8_t function_;
    };
    std::ostream& operator<<(std::ostream& os, const Device& d);

    extern std::list<Device> devices;
    void init();
  }
}

#endif
