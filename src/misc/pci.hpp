#ifndef EBBRT_MISC_PCI_HPP
#define EBBRT_MISC_PCI_HPP

#include <cstdint>
#include <ostream>

namespace ebbrt {
  namespace pci {
    void enumerate_bus(std::ostream& ostream, uint8_t bus);
    void enumerate_all_buses(std::ostream& ostream);
    class Function {
    public:
      Function(uint8_t bus, uint8_t device, uint8_t function);
      uint16_t DeviceId();
      uint16_t VendorId();
      uint16_t Status();
      uint16_t Command();
      uint8_t ClassCode();
      static const uint8_t ClassCode_BridgeDevice = 0x06;
      uint8_t Subclass();
      static const uint8_t Subclass_PCItoPCIBridge = 0x04;
      uint8_t ProgIF();
      uint8_t RevisionId();
      uint8_t BIST();
      uint8_t HeaderType();
      uint8_t LatencyTimer();
      uint8_t CacheLineSize();

      bool Valid();
      bool MultiFunction();
      bool GeneralHeaderType();
      void Enumerate(std::ostream& ostream);

      // GeneralHeaderType ONLY!
      uint32_t BAR0();
      uint32_t BAR1();
      uint32_t BAR2();
      uint32_t BAR3();
      uint32_t BAR4();
      uint32_t BAR5();
      uint32_t CardbusCISPointer();
      uint16_t SubsystemId();
      uint16_t SubsystemVendorId();
      uint32_t ExpansionROMBaseAddress();
      uint8_t Capabilities();
      uint8_t MaxLatency();
      uint8_t MinGrant();
      uint8_t InterruptPIN();
      uint8_t InterruptLine();
    private:
      uint8_t read8(uint8_t offset);
      uint16_t read16(uint8_t offset);
      uint32_t read32(uint8_t offset);

      uint8_t bus_;
      uint8_t device_;
      uint8_t function_;
    };
  }
}

#endif
