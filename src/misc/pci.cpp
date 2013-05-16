#include "arch/io.hpp"
#include "misc/pci.hpp"

namespace {
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
}

ebbrt::pci::Function::Function(uint8_t bus, uint8_t device, uint8_t function)
  : bus_(bus), device_(device), function_(function)
{
}

uint16_t
ebbrt::pci::Function::DeviceId()
{
  return read16(0x00);
}

uint16_t
ebbrt::pci::Function::VendorId()
{
  return read16(0x02);
}

uint16_t
ebbrt::pci::Function::Status()
{
  return read16(0x04);
}

uint16_t
ebbrt::pci::Function::Command()
{
  return read16(0x06);
}

uint8_t
ebbrt::pci::Function::ClassCode()
{
  return read8(0x08);
}

uint8_t
ebbrt::pci::Function::Subclass()
{
  return read8(0x09);
}

uint8_t
ebbrt::pci::Function::ProgIF()
{
  return read8(0x0a);
}

uint8_t
ebbrt::pci::Function::RevisionId()
{
  return read8(0x0b);
}

uint8_t
ebbrt::pci::Function::BIST()
{
  return read8(0x0c);
}

uint8_t
ebbrt::pci::Function::HeaderType()
{
  return read8(0x0d);
}

uint8_t
ebbrt::pci::Function::LatencyTimer()
{
  return read8(0x0e);
}

uint8_t
ebbrt::pci::Function::CacheLineSize()
{
  return read8(0x0f);
}

uint32_t
ebbrt::pci::Function::BAR0()
{
  return read32(0x10);
}

uint32_t
ebbrt::pci::Function::BAR1()
{
  return read32(0x14);
}

uint32_t
ebbrt::pci::Function::BAR2()
{
  return read32(0x18);
}

uint32_t
ebbrt::pci::Function::BAR3()
{
  return read32(0x1c);
}

uint32_t
ebbrt::pci::Function::BAR4()
{
  return read32(0x20);
}

uint32_t
ebbrt::pci::Function::BAR5()
{
  return read32(0x24);
}

uint32_t
ebbrt::pci::Function::CardbusCISPointer()
{
  return read32(0x28);
}

uint16_t
ebbrt::pci::Function::SubsystemId()
{
  return read16(0x2c);
}

uint16_t
ebbrt::pci::Function::SubsystemVendorId()
{
  return read16(0x2e);
}

uint32_t
ebbrt::pci::Function::ExpansionROMBaseAddress()
{
  return read32(0x30);
}

uint8_t
ebbrt::pci::Function::Capabilities()
{
  return read8(0x37);
}

uint8_t
ebbrt::pci::Function::MaxLatency()
{
  return read8(0x3c);
}

uint8_t
ebbrt::pci::Function::MinGrant()
{
  return read8(0x3d);
}

uint8_t
ebbrt::pci::Function::InterruptPIN()
{
  return read8(0x3e);
}

uint8_t
ebbrt::pci::Function::InterruptLine()
{
  return read8(0x3f);
}

bool
ebbrt::pci::Function::Valid()
{
  return VendorId() != 0xFFFF;
}

bool
ebbrt::pci::Function::MultiFunction()
{
  return HeaderType() & (1 << 7);
}

bool
ebbrt::pci::Function::GeneralHeaderType()
{
  return (HeaderType() & 0x7f) == 0x00;
}

void
ebbrt::pci::Function::Enumerate(std::ostream& ostream)
{
  ostream << "Bus: " << static_cast<int>(bus_) <<
    ", Device: " << static_cast<int>(device_) <<
    ", Function: " << static_cast<int>(function_) << std::endl;
  ostream << std::hex;
  ostream << "Device ID: 0x" << DeviceId() << std::endl;
  ostream << "Vendor ID: 0x" << VendorId() << std::endl;
  ostream << "Status: 0x" << Status() << std::endl;
  ostream << "Command: 0x" << Command() << std::endl;
  ostream << "Class code: 0x" << static_cast<int>(ClassCode()) << std::endl;
  ostream << "Subclass: 0x" << static_cast<int>(Subclass()) << std::endl;
  ostream << "Prog IF: 0x" << static_cast<int>(ProgIF()) << std::endl;
  ostream << "Revision ID: 0x" << static_cast<int>(RevisionId()) <<
    std::endl;
  ostream << "BIST: 0x" << static_cast<int>(BIST()) << std::endl;
  ostream << "Header type: 0x" << static_cast<int>(HeaderType()) <<
    std::endl;
  ostream << "Latency Timer: 0x" << static_cast<int>(LatencyTimer()) <<
    std::endl;
  ostream << "Cache Line Size: 0x" << static_cast<int>(CacheLineSize()) <<
    std::endl;

  if (GeneralHeaderType()) {
    ostream << "BAR0: 0x" << BAR0() << std::endl;
    ostream << "BAR1: 0x" << BAR1() << std::endl;
    ostream << "BAR2: 0x" << BAR2() << std::endl;
    ostream << "BAR3: 0x" << BAR3() << std::endl;
    ostream << "BAR4: 0x" << BAR4() << std::endl;
    ostream << "BAR5: 0x" << BAR5() << std::endl;
    ostream << "Cardbus CIS Pointer: 0x" << CardbusCISPointer() << std::endl;
    ostream << "Subsystem Id: 0x" << SubsystemId() << std::endl;
    ostream << "Subsystem Vendor Id: 0x" << SubsystemVendorId() << std::endl;
    ostream << "Expansion ROM base address: 0x" << ExpansionROMBaseAddress() <<
      std::endl;
    ostream << "Capabilities Pointer: 0x" <<
      static_cast<int>(Capabilities()) << std::endl;
    ostream << "Max Latency: 0x" << static_cast<int>(MaxLatency()) << std::endl;
    ostream << "Min Grant: 0x" << static_cast<int>(MinGrant()) << std::endl;
    ostream << "Interrupt PIN: 0x" << static_cast<int>(InterruptPIN()) << std::endl;
    ostream << "InterruptLine: 0x" << static_cast<int>(InterruptLine()) << std::endl;
  } else {
    ostream << "Unimplemented Header Type" << std::endl;
  }

  ostream << std::dec;
  // If we are a pci bridge, then enumerate recursively
  if (ClassCode() == ClassCode_BridgeDevice &&
      Subclass() == Subclass_PCItoPCIBridge) {
    // We need to find the secondary bus and enumerate it
    ostream << "Unimplemented PCI bridge" << std::endl;
  }
}

uint8_t
ebbrt::pci::Function::read8(uint8_t offset)
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

  return val >> ((offset & 2) * 8); //offset return value
}

uint16_t
ebbrt::pci::Function::read16(uint8_t offset)
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

  return val >> ((offset & 2) * 8); //offset return value
}

uint32_t
ebbrt::pci::Function::read32(uint8_t offset)
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

void
ebbrt::pci::enumerate_bus(std::ostream& ostream, uint8_t bus)
{
  // iterate over all devices on this bus
  for (uint8_t device = 0; device < 32; ++device) {
    pci::Function dev(bus, device, 0);
    if (!dev.Valid()) {
      continue;
    }

    // we have a valid device
    dev.Enumerate(ostream);

    // if it is multifunction, enumerate the rest of the valid
    // functions
    if (dev.MultiFunction()) {
      for (uint8_t function = 1; function < 8; ++function) {
        pci::Function dev2(bus, device, function);
        if (dev2.Valid()) {
          dev2.Enumerate(ostream);
        }
      }
    }
  }
}

void
ebbrt::pci::enumerate_all_buses(std::ostream& ostream)
{
  pci::Function bus(0, 0, 0);

  // If the first device is multifunction then there are multiple host
  // controllers each responsible for a bus, otherwise only one bus
  if (bus.MultiFunction()) {
    for (uint8_t function = 0; function < 8; ++function) {
      pci::Function dev(0, 0, function);
      if (dev.Valid()) {
        enumerate_bus(ostream, function);
      }
    }
  } else {
    enumerate_bus(ostream, 0);
  }
}
