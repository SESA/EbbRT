//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <ebbrt/Cpuid.h>

#include <cstdint>
#include <cstring>

namespace {
struct Result {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
};

Result Cpuid(uint32_t leaf) {
  Result r;
  asm("cpuid" : "=a"(r.eax), "=b"(r.ebx), "=c"(r.ecx), "=d"(r.edx) : "a"(leaf));
  return r;
}

struct VendorId {
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
};

VendorId kvm_vendor_id = {0x4b4d564b, 0x564b4d56, 0x4d};

struct CpuidBit {
  uint32_t leaf;
  uint8_t reg;
  uint32_t bit;
  bool ebbrt::cpuid::Features::*flag;
  VendorId* vendor_id;
};

CpuidBit cpuid_bits[] = {
    {1, 2, 21, &ebbrt::cpuid::Features::x2apic},
    {0x40000001, 0, 6, &ebbrt::cpuid::Features::kvm_pv_eoi, &kvm_vendor_id},
    {0x40000001, 0, 3, &ebbrt::cpuid::Features::kvm_clocksource2,
     &kvm_vendor_id}};

constexpr size_t nr_cpuid_bits = sizeof(cpuid_bits) / sizeof(CpuidBit);
}  // namespace

ebbrt::cpuid::Features ebbrt::cpuid::features;

void ebbrt::cpuid::Init() {
  for (size_t i = 0; i < nr_cpuid_bits; ++i) {
    const auto& bit = cpuid_bits[i];
    auto vals = Cpuid(bit.leaf & 0xf0000000);
    if (bit.vendor_id) {
      if (vals.ebx != bit.vendor_id->ebx || vals.ecx != bit.vendor_id->ecx ||
          vals.edx != bit.vendor_id->edx) {
        continue;
      }
    }
    if ((bit.leaf & 0xf0000000) == 0x40000000 &&
        bit.vendor_id == &kvm_vendor_id && vals.eax == 0) {
      vals.eax = 0x40000001;  // kvm bug workaround
    }
    if (bit.leaf > vals.eax) {
      continue;
    }

    auto res = Cpuid(bit.leaf);
    uint32_t res_array[4] = {res.eax, res.ebx, res.ecx, res.edx};
    uint32_t val = res_array[bit.reg];
    features.*(bit.flag) = (val >> bit.bit) & 1;
  }
}
