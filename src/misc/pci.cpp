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

ebbrt::pci::Device::Device(uint8_t bus, uint8_t device, uint8_t function)
  : bus_(bus), device_(device), function_(function)
{
}

uint16_t
ebbrt::pci::Device::VendorId() const
{
  return read16(0x00);
}

uint16_t
ebbrt::pci::Device::DeviceId() const
{
  return read16(0x02);
}

uint16_t
ebbrt::pci::Device::Command() const
{
  return read16(0x04);
}

uint16_t
ebbrt::pci::Device::Status() const
{
  return read16(0x06);
}

uint8_t
ebbrt::pci::Device::RevisionId() const
{
  return read8(0x08);
}

uint8_t
ebbrt::pci::Device::ProgIF() const
{
  return read8(0x09);
}

uint8_t
ebbrt::pci::Device::Subclass() const
{
  return read8(0x0a);
}

uint8_t
ebbrt::pci::Device::ClassCode() const
{
  return read8(0x0b);
}

uint8_t
ebbrt::pci::Device::CacheLineSize() const
{
  return read8(0x0c);
}

uint8_t
ebbrt::pci::Device::LatencyTimer() const
{
  return read8(0x0d);
}

uint8_t
ebbrt::pci::Device::HeaderType() const
{
  return read8(0x0e);
}

uint8_t
ebbrt::pci::Device::BIST() const
{
  return read8(0x0f);
}

uint32_t
ebbrt::pci::Device::BAR0() const
{
  return read32(0x10);
}

uint32_t
ebbrt::pci::Device::BAR1() const
{
  return read32(0x14);
}

uint32_t
ebbrt::pci::Device::BAR2() const
{
  return read32(0x18);
}

uint32_t
ebbrt::pci::Device::BAR3() const
{
  return read32(0x1c);
}

uint32_t
ebbrt::pci::Device::BAR4() const
{
  return read32(0x20);
}

uint32_t
ebbrt::pci::Device::BAR5() const
{
  return read32(0x24);
}

uint32_t
ebbrt::pci::Device::CardbusCISPointer() const
{
  return read32(0x28);
}

uint16_t
ebbrt::pci::Device::SubsystemVendorId() const
{
  return read16(0x2c);
}

uint16_t
ebbrt::pci::Device::SubsystemId() const
{
  return read16(0x2e);
}

uint32_t
ebbrt::pci::Device::ExpansionROMBaseAddress() const
{
  return read32(0x30);
}

uint8_t
ebbrt::pci::Device::Capabilities() const
{
  return read8(0x34);
}

uint8_t
ebbrt::pci::Device::InterruptLine() const
{
  return read8(0x3c);
}

uint8_t
ebbrt::pci::Device::InterruptPIN() const
{
  return read8(0x3d);
}

uint8_t
ebbrt::pci::Device::MinGrant() const
{
  return read8(0x3e);
}

uint8_t
ebbrt::pci::Device::MaxLatency() const
{
  return read8(0x3f);
}

bool
ebbrt::pci::Device::Valid() const
{
  return VendorId() != 0xFFFF;
}

bool
ebbrt::pci::Device::MultiFunction() const
{
  return HeaderType() & (1 << 7);
}

bool
ebbrt::pci::Device::GeneralHeaderType() const
{
  return (HeaderType() & 0x7f) == 0x00;
}

std::ostream&
ebbrt::pci::operator<<(std::ostream& os, const ebbrt::pci::Device& d)
{
  os << "Bus: " << static_cast<int>(d.bus_) <<
    ", Device: " << static_cast<int>(d.device_) <<
    ", Function: " << static_cast<int>(d.function_) << std::endl;
  os << std::hex;
  os << "Device ID: 0x" << d.DeviceId() << std::endl;
  os << "Vendor ID: 0x" << d.VendorId() << std::endl;
  os << "Status: 0x" << d.Status() << std::endl;
  os << "Command: 0x" << d.Command() << std::endl;
  os << "Class code: 0x" << static_cast<int>(d.ClassCode()) << std::endl;
  os << "Subclass: 0x" << static_cast<int>(d.Subclass()) << std::endl;
  os << "Prog IF: 0x" << static_cast<int>(d.ProgIF()) << std::endl;
  os << "Revision ID: 0x" << static_cast<int>(d.RevisionId()) <<
    std::endl;
  os << "BIST: 0x" << static_cast<int>(d.BIST()) << std::endl;
  os << "Header type: 0x" << static_cast<int>(d.HeaderType()) <<
    std::endl;
  os << "Latency Timer: 0x" << static_cast<int>(d.LatencyTimer()) <<
    std::endl;
  os << "Cache Line Size: 0x" << static_cast<int>(d.CacheLineSize()) <<
    std::endl;

  if (d.GeneralHeaderType()) {
    os << "BAR0: 0x" << d.BAR0() << std::endl;
    os << "BAR1: 0x" << d.BAR1() << std::endl;
    os << "BAR2: 0x" << d.BAR2() << std::endl;
    os << "BAR3: 0x" << d.BAR3() << std::endl;
    os << "BAR4: 0x" << d.BAR4() << std::endl;
    os << "BAR5: 0x" << d.BAR5() << std::endl;
    os << "Cardbus CIS Pointer: 0x" << d.CardbusCISPointer() << std::endl;
    os << "Subsystem Id: 0x" << d.SubsystemId() << std::endl;
    os << "Subsystem Vendor Id: 0x" << d.SubsystemVendorId() << std::endl;
    os << "Expansion ROM base address: 0x" << d.ExpansionROMBaseAddress() <<
      std::endl;
    os << "Capabilities Pointer: 0x" <<
      static_cast<int>(d.Capabilities()) << std::endl;
    os << "Max Latency: 0x" << static_cast<int>(d.MaxLatency()) << std::endl;
    os << "Min Grant: 0x" << static_cast<int>(d.MinGrant()) << std::endl;
    os << "Interrupt PIN: 0x" << static_cast<int>(d.InterruptPIN()) << std::endl;
    os << "InterruptLine: 0x" << static_cast<int>(d.InterruptLine()) << std::endl;
  } else {
    os << "Unimplemented Header Type" << std::endl;
  }

  os << std::dec;
  // If we are a pci bridge, then enumerate recursively
  if (d.ClassCode() == ebbrt::pci::Device::ClassCode_BridgeDevice &&
      d.Subclass() == ebbrt::pci::Device::Subclass_PCItoPCIBridge) {
    // We need to find the secondary bus and enumerate it
    os << "Unimplemented PCI bridge" << std::endl;
  }
  return os;
}

uint8_t
ebbrt::pci::Device::read8(uint8_t offset) const
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
ebbrt::pci::Device::read16(uint8_t offset) const
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
ebbrt::pci::Device::read32(uint8_t offset) const
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

namespace {
  void
  enumerate_bus(uint8_t bus) {
    using namespace ebbrt::pci;
    // iterate over all devices on this bus
    for (uint8_t device = 0; device < 32; ++device) {
      Device dev(bus, device, 0);
      if (!dev.Valid()) {
        continue;
      }

      // we have a valid device
      devices.push_back(dev);

      //FIXME: should check if this is a bridge and enumerate behind it
    }
  }
}

std::list<ebbrt::pci::Device> ebbrt::pci::devices;

void
ebbrt::pci::init()
{
  Device bus(0, 0, 0);

  if (bus.MultiFunction()) {
    for (uint8_t function = 0; function < 8; ++function) {
      Device dev(0, 0, function);
      if (dev.Valid()) {
        enumerate_bus(function);
      }
    }
  } else {
    enumerate_bus(0);
  }
}
