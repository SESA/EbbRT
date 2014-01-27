//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Apic.h>

#include <ebbrt/Cpu.h>
#include <ebbrt/Msr.h>

void ebbrt::apic::Init() {
  auto apic_base = msr::Read(msr::kIa32ApicBase);
  // enable x2apic mode
  msr::Write(msr::kIa32ApicBase, apic_base | 0xc00);
  msr::Write(msr::kX2apicSvr, 0x100);
}

void ebbrt::apic::Ipi(uint8_t apic_id, uint8_t vector, bool level,
                      uint8_t delivery_mode) {
  auto val = (uint64_t(apic_id) << 32) | (uint64_t(level) << 14) |
             (uint64_t(delivery_mode) << 8) | vector;
  msr::Write(msr::kX2apicIcr, val);
}

uint32_t ebbrt::apic::GetId() { return msr::Read(msr::kX2apicIdr); }

void ebbrt::apic::Eoi() {
  // TODO(dschatz): use KVM pv eoi
  msr::Write(msr::kX2apicEoi, 0);
}
