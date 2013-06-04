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
#ifndef EBBRT_ARCH_X86_64_ACPI_HPP
#define EBBRT_ARCH_X86_64_ACPI_HPP

#include <cstdint>
#include <cstring>

namespace ebbrt {
  namespace acpi {
    void init();
    unsigned get_num_cores();
    unsigned get_num_io_apics();
    class Header {
    public:
      char signature[4];
      uint32_t len;
      uint8_t revision;
      uint8_t checksum;
      char oemId[6];
      char oemTid[8];
      uint32_t oemRevision;
      uint32_t creatorId;
      uint32_t creatorRev;
    } __attribute__((packed));

    template<typename T>
    class Sdt {
      Header header;
      T ptr[0];
    public:
      const Header *find(const char signature[4]) const {
        for (unsigned int i = 0;
             i < ((header.len-sizeof(header))/sizeof(T));
             i++) {
          const Header* h = reinterpret_cast<const Header*>(ptr[i]);
          if(h->signature[0] == signature[0] &&
             h->signature[1] == signature[1] &&
             h->signature[2] == signature[2] &&
             h->signature[3] == signature[3]) {
            return h;
          }
        }
        return nullptr;
      }
    } __attribute__((packed));
    typedef Sdt<uint32_t> Rsdt;
    typedef Sdt<uint64_t> Xsdt;

    class Rsdp {
    public:
      static const Rsdp *locate();
      const Rsdt* getRsdt() const;
      const Xsdt* getXsdt() const;
    private:
      char signature[8];
      uint8_t checksum;
      char oemid[6];
      uint8_t revision;
      uint32_t rsdtAddress;
      uint32_t length;
      uint64_t xsdtAddress;
      uint8_t extChecksum;
      uint8_t reserved[3];
      static const Rsdp *locateInRange(const char *start, int length);
    } __attribute__((packed));

    class MadtHeader {
    public:
      uint8_t type;
      uint8_t len;
      static const uint8_t LAPIC_TYPE = 0;
      static const uint8_t IOAPIC_TYPE = 1;
    } __attribute__((packed));

    class MadtLapic {
    private:
      MadtHeader header;
    public:
      uint8_t acpiId;
      uint8_t apicId;
      struct {
        uint32_t enabled :1;
      uint32_t :31;
      };
    } __attribute__((packed));

    class MadtIoApic {
    private:
      MadtHeader header;
    public:
      uint8_t ioApicId;
      uint8_t reserved;
      uint32_t addr;
      uint32_t intBase;
    } __attribute__((packed));

    const MadtIoApic* find_io_apic(int i);

    class Madt {
    public:
      const MadtHeader* find(uint8_t type, int index) const;
      const MadtLapic* findLapic(int index) const;
      const MadtIoApic* findIoApic(int index) const;
    private:
      Header header;
      uint32_t lapicAddr;
      uint32_t flags;
      uint8_t data[0];
    } __attribute__((packed));

    class SratHeader {
    public:
      uint8_t type;
      uint8_t len;
      static const uint8_t LAPIC_TYPE = 0;
      static const uint8_t MEM_AFFINITY_TYPE = 1;
      static const uint8_t X2APIC_TYPE = 2;
    } __attribute__((packed));

    class SratLapic {
    private:
      SratHeader header;
    public:
      uint8_t proximityDomain_0_7;
      uint8_t apicId;
      struct {
        uint32_t enabled :1;
      uint32_t :31;
      };
      uint8_t sapicId;
      uint8_t proximityDomain_8_31[3];
      uint32_t clockDomain;
    };

    class SratMemAffinity {
    private:
      SratHeader header;
    public:
      uint32_t proximityDomain;
      uint16_t reserved0;
      uint32_t baseAddressLow;
      uint32_t baseAddressHigh;
      uint32_t lengthLow;
      uint32_t lengthHigh;
      uint32_t reserved1;
      struct {
        uint32_t enabled :1;
        uint32_t hotPluggable :1;
        uint32_t nonVolatile :1;
      uint32_t :29;
      };
      uint64_t reserved2;
    };

    class SratX2Apic {
    private:
      SratHeader header;
    public:
      uint16_t reserved0;
      uint32_t proxDomain;
      uint32_t x2ApicId;
      struct {
        uint32_t enabled :1;
      uint32_t :31;
      };
      uint32_t clockDomain;
      uint32_t reserved1;
    };

    class Srat {
    public:
      const SratHeader* find(uint8_t type, int index) const;
      const SratLapic* findLapic(int index) const;
      const SratMemAffinity* findMemAffinity(int index) const;
      const SratX2Apic* findX2Apic(int index) const;
    private:
      Header header;
      uint32_t reserved0;
      uint64_t reserved1;
      uint8_t data[0];
    } __attribute__((packed));

    class CoreInfo {
    public:
      uint32_t proximityDomain;
      uint8_t apicId;
    };

    class ProximityDomain {
    public:
      uint64_t baseAddress;
      uint64_t length;
      uint32_t proximityDomain;
      int nCores;
    };
  };
}

#endif
