//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Apic.h>

#include <ebbrt/Cpu.h>
#include <ebbrt/Cpuid.h>
#include <ebbrt/Msr.h>

void ebbrt::apic::Init() {
  auto apic_base = msr::Read(msr::kIa32ApicBase);
  // enable x2apic mode
  msr::Write(msr::kIa32ApicBase, apic_base | 0xc00);
  msr::Write(msr::kX2apicSvr, 0x100);
}

namespace {
std::array<uint32_t, ebbrt::Cpu::kMaxCpus> eoi_words;

bool PvEoi() {
  uint8_t ret;
  asm volatile("btr %[zero], %[eoi_word];"
               "setc %[ret]"
               : [eoi_word] "+m"(eoi_words[ebbrt::Cpu::GetMine()]),
                 [ret] "=rm"(ret)
               : [zero] "r"(0));
  return ret;
}
}  // namespace

void ebbrt::apic::PVEoiInit(size_t cpu) {
  if (cpuid::features.kvm_pv_eoi) {
    msr::Write(msr::kKvmPvEoi,
               reinterpret_cast<uintptr_t>(&eoi_words[cpu]) | 1);
  }
}

void ebbrt::apic::Ipi(uint8_t apic_id, uint8_t vector, bool level,
                      uint8_t delivery_mode) {
  auto val = (uint64_t(apic_id) << 32) | (uint64_t(level) << 14) |
             (uint64_t(delivery_mode) << 8) | vector;
  msr::Write(msr::kX2apicIcr, val);
}

uint32_t ebbrt::apic::GetId() { return msr::Read(msr::kX2apicIdr); }

void ebbrt::apic::Eoi() {
  if (PvEoi())
    return;

  msr::Write(msr::kX2apicEoi, 0);
}
