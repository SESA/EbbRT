//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_IDT_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_IDT_H_

#include <cstdint>

namespace ebbrt {
namespace idt {
struct ExceptionFrame {
  uint64_t fpu[64];
  uint64_t empty;
  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t r11;
  uint64_t r10;
  uint64_t r9;
  uint64_t r8;
  uint64_t rbp;
  uint64_t rdi;
  uint64_t rsi;
  uint64_t rdx;
  uint64_t rcx;
  uint64_t rbx;
  uint64_t rax;
  uint64_t error_code;
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
};

void Init();
void Load();
extern "C" __attribute__((noreturn)) void EventInterrupt(int num);
extern "C" void NmiInterrupt(ExceptionFrame* ef) __attribute__((weak));
extern "C" void DivideErrorException(ExceptionFrame* ef) __attribute__((weak));
extern "C" void DebugException(ExceptionFrame* ef) __attribute__((weak));
extern "C" void BreakpointException(ExceptionFrame* ef) __attribute__((weak));
extern "C" void OverflowException(ExceptionFrame* ef) __attribute__((weak));
extern "C" void BoundRangeExceededException(ExceptionFrame* ef) __attribute__((weak));
extern "C" void InvalidOpcodeException(ExceptionFrame* ef) __attribute__((weak));
extern "C" void DeviceNotAvailableException(ExceptionFrame* ef) __attribute__((weak));
extern "C" void DoubleFaultException(ExceptionFrame* ef) __attribute__((weak));
extern "C" void InvalidTssException(ExceptionFrame* ef) __attribute__((weak));
extern "C" void SegmentNotPresent(ExceptionFrame* ef) __attribute__((weak));
extern "C" void StackFaultException(ExceptionFrame* ef) __attribute__((weak));
extern "C" void GeneralProtectionException(ExceptionFrame* ef) __attribute__((weak));
extern "C" void PageFaultException(ExceptionFrame* ef) __attribute__((weak));
extern "C" void X87FpuFloatingPointError(ExceptionFrame* ef) __attribute__((weak));
extern "C" void AlignmentCheckException(ExceptionFrame* ef) __attribute__((weak));
extern "C" void MachineCheckException(ExceptionFrame* ef) __attribute__((weak));
extern "C" void SimdFloatingPointException(ExceptionFrame* ef) __attribute__((weak));
}  // namespace idt
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_IDT_H_
