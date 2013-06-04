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
#include "ebb/SharedRoot.hpp"
#include "ebb/PCI/PCI.hpp"

ebbrt::EbbRef<ebbrt::PCI> ebbrt::pci;

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
