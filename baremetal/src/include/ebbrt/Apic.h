//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_APIC_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_APIC_H_

#include <cstdint>

namespace ebbrt {
namespace apic {
void Init();
const constexpr uint8_t kDeliveryFixed = 0;
const constexpr uint8_t kDeliverySmi = 2;
const constexpr uint8_t kDeliveryNmi = 4;
const constexpr uint8_t kDeliveryInit = 5;
const constexpr uint8_t kDeliveryStartup = 6;

void Ipi(uint8_t apic_id, uint8_t vector, bool level = true,
         uint8_t delivery_mode = kDeliveryFixed);
void PVEoiInit(std::size_t cpu);
uint32_t GetId();
void Eoi();
}  // namespace apic
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_APIC_H_
