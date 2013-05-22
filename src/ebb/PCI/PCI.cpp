#include "ebb/SharedRoot.hpp"
#include "ebb/PCI/PCI.hpp"

ebbrt::Ebb<ebbrt::PCI> ebbrt::pci;

ebbrt::EbbRoot*
ebbrt::PCI::ConstructRoot()
{
  return new SharedRoot<PCI>;
}

ebbrt::PCI::PCI()
{
  Device bus(0, 0, 0);

  if (bus.MultiFunction()) {
    for (uint8_t function = 0; function < 8; ++function) {
      Device dev(0, 0, function);
      if (dev.Valid()) {
        EnumerateBus(function);
      }
    }
  } else {
    EnumerateBus(0);
  }
}

std::list<ebbrt::PCI::Device>::iterator
ebbrt::PCI::DevicesBegin()
{
  return devices_.begin();
}

std::list<ebbrt::PCI::Device>::iterator
ebbrt::PCI::DevicesEnd()
{
  return devices_.end();
}

void
ebbrt::PCI::EnumerateBus(uint8_t bus) {
  // iterate over all devices on this bus
  for (uint8_t device = 0; device < 32; ++device) {
    Device dev(bus, device, 0);
    if (!dev.Valid()) {
      continue;
    }

    // we have a valid device
    devices_.push_back(dev);

    //FIXME: should check if this is a bridge and enumerate behind it
  }
}
