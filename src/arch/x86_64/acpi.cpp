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
#include <stdexcept>

#include "arch/x86_64/acpi.hpp"

const ebbrt::acpi::Rsdp* rsdp;
const ebbrt::acpi::Rsdt* rsdt;
const ebbrt::acpi::Xsdt* xsdt;
const ebbrt::acpi::Madt* madt;
const ebbrt::acpi::Srat* srat;

void
ebbrt::acpi::init()
{
  rsdp = Rsdp::locate();

  if (rsdp != nullptr) {
    xsdt = rsdp->getXsdt();
    if (xsdt != nullptr &&
        reinterpret_cast<uintptr_t>(xsdt) <= 0xFFFFFFFF) {
        madt = reinterpret_cast<const Madt*>(xsdt->find("APIC"));
        srat = reinterpret_cast<const Srat*>(xsdt->find("SRAT"));
    } else {
      rsdt = rsdp->getRsdt();
      if (rsdt != nullptr &&
          reinterpret_cast<uintptr_t>(rsdt) <= 0xFFFFFFFF) {
        madt = reinterpret_cast<const Madt*>(rsdt->find("APIC"));
        srat = reinterpret_cast<const Srat*>(rsdt->find("SRAT"));
      }
    }
  }
}

unsigned
ebbrt::acpi::get_num_cores()
{
  if (madt == nullptr &&
      reinterpret_cast<uintptr_t>(madt) > 0xFFFFFFFF) {
    return 0;
  }
  const MadtLapic* madtLapic;
  int ret = 0;
  for (int i = 0; (madtLapic = madt->findLapic(i)) != nullptr; i++) {
    if (madtLapic->enabled) {
      ret++;
    }
  }
  return ret;
}

unsigned
ebbrt::acpi::get_num_io_apics()
{
  if (madt == nullptr &&
      reinterpret_cast<uintptr_t>(madt) > 0xFFFFFFFF) {
    return 0;
  }
  const MadtIoApic* madtIoApic;
  int ret = 0;
  for (int i = 0; (madtIoApic = madt->findIoApic(i)) != nullptr; i++) {
    ret++;
  }
  return ret;
}

namespace {
  const volatile uint16_t* BDA_EBDA =
    reinterpret_cast<volatile uint16_t*>(0x40e);
  const char* SECOND_SEARCH_LOCATION = reinterpret_cast<char*>(0xe0000);
  const int SECOND_SEARCH_SIZE = 0x20000;

}

const ebbrt::acpi::MadtIoApic*
ebbrt::acpi::find_io_apic(int i) {
  return madt->findIoApic(i);
}

const ebbrt::acpi::Rsdp*
ebbrt::acpi::Rsdp::locateInRange(const char* start, int length) {
  for (const char* p = start;
       p < (start + length);
       p += 16) {
    if (p[0] == 'R' &&
        p[1] == 'S' &&
        p[2] == 'D' &&
        p[3] == ' ' &&
        p[4] == 'P' &&
        p[5] == 'T' &&
        p[6] == 'R' &&
        p[7] == ' ') {
      return reinterpret_cast<const Rsdp*>(p);
    }
  }
  return nullptr;
}

const ebbrt::acpi::Rsdp*
ebbrt::acpi::Rsdp::locate() {
  char *ebda = reinterpret_cast<char *>
    (static_cast<uintptr_t>(*BDA_EBDA) << 4);
  const Rsdp* ret = nullptr;
  if((ret = locateInRange(ebda, 1 << 10)) == nullptr) {
    ret = locateInRange(SECOND_SEARCH_LOCATION, SECOND_SEARCH_SIZE);
  }
  return ret;
}

const ebbrt::acpi::Rsdt*
ebbrt::acpi::Rsdp::getRsdt() const
{
  return reinterpret_cast<Rsdt*>(rsdtAddress);
}

const ebbrt::acpi::Xsdt*
ebbrt::acpi::Rsdp::getXsdt() const
{
  return reinterpret_cast<Xsdt*>(xsdtAddress);
}

const ebbrt::acpi::MadtHeader*
ebbrt::acpi::Madt::find(uint8_t type, int index) const
{
  for (unsigned i = 0; i < (header.len-sizeof(Madt));) {
    const MadtHeader* h = reinterpret_cast<const MadtHeader*>(&data[i]);
    if (h->type == type) {
      if (index == 0) {
        return h;
      }
      index--;
    }
    i += h->len;
  }
  return nullptr;
}

const ebbrt::acpi::MadtLapic*
ebbrt::acpi::Madt::findLapic(int index) const
{
  return reinterpret_cast<const MadtLapic*>(find(MadtHeader::LAPIC_TYPE,
                                                 index));
}

const ebbrt::acpi::MadtIoApic*
ebbrt::acpi::Madt::findIoApic(int index) const
{
  return reinterpret_cast<const MadtIoApic*>(find(MadtHeader::IOAPIC_TYPE,
                                                  index));
}

const ebbrt::acpi::SratHeader*
ebbrt::acpi::Srat::find(uint8_t type, int index) const
{
  for (unsigned i = 0; i < (header.len-sizeof(Srat));) {
    const SratHeader* h = reinterpret_cast<const SratHeader*>(&data[i]);
    if (h->type == type) {
      if (index == 0) {
        return h;
      }
      index--;
    }
    i += h->len;
  }
  return nullptr;
}

const ebbrt::acpi::SratLapic*
ebbrt::acpi::Srat::findLapic(int index) const
{
  return reinterpret_cast<const SratLapic*>(find(SratHeader::LAPIC_TYPE,
                                                 index));
}

const ebbrt::acpi::SratMemAffinity*
ebbrt::acpi::Srat::findMemAffinity(int index) const
{
  return
    reinterpret_cast<const SratMemAffinity*>(find(SratHeader::MEM_AFFINITY_TYPE,
                                                  index));
}
