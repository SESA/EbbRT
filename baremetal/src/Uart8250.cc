//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Console.h>

#include <mutex>

#include <ebbrt/ExplicitlyConstructed.h>
#include <ebbrt/Io.h>
#include <ebbrt/SpinLock.h>

namespace {
const constexpr uint16_t kPort = 0x3f8;
// when DLAB = 0
const constexpr uint16_t kDataReg = 0;
const constexpr uint16_t kIntEnable = 1;
// when DLAB = 1
// Least-signficant byte of the baud rate divisor
const constexpr uint16_t kBaudDivLsb = 0;
// Most-signficant byte of the baud rate divisor
const constexpr uint16_t kBaudDivMsb = 1;

const constexpr uint16_t kLineCntlReg = 3;
const constexpr uint8_t kLineCntlRegCharlen8 = 1 << 0 | 1 << 1;
const constexpr uint8_t kLineCntlRegDlab = 1 << 7;

const constexpr uint16_t kLineStatusReg = 5;
const constexpr uint8_t kLineStatusRegThrEmpty = 1 << 5;

ebbrt::ExplicitlyConstructed<ebbrt::SpinLock> console_lock;
}  // namespace

void ebbrt::console::Init() noexcept {
  io::Out8(kPort + kIntEnable, 0);  // disable interrupts

  io::Out8(kPort + kLineCntlReg, kLineCntlRegDlab);  // enable dlab
  // set divisor to 1 (115200 baud)
  io::Out8(kPort + kBaudDivLsb, 1);
  io::Out8(kPort + kBaudDivMsb, 0);

  // set as 8N1 (8 bits, no parity, one stop bit)
  io::Out8(kPort + kLineCntlReg, kLineCntlRegCharlen8);

  console_lock.construct();
}

namespace {
void WriteLocked(char c) {
  while (!(ebbrt::io::In8(kPort + kLineStatusReg) & kLineStatusRegThrEmpty)) {
  }

  ebbrt::io::Out8(kPort + kDataReg, c);
}
}

void ebbrt::console::Write(const char *str) noexcept {
  std::lock_guard<SpinLock> lock(*console_lock);
  while (*str != '\0') {
    WriteLocked(*str++);
  }
}
