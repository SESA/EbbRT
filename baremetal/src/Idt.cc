//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Idt.h>

#include <cinttypes>

#include <ebbrt/Debug.h>
#include <ebbrt/EventManager.h>

extern char EventEntry[];

extern "C" void IntDe();
extern "C" void IntDb();
extern "C" void IntNmi();
extern "C" void IntBp();
extern "C" void IntOf();
extern "C" void IntBr();
extern "C" void IntUd();
extern "C" void IntNm();
extern "C" void IntDf();
extern "C" void IntTs();
extern "C" void IntNp();
extern "C" void IntSs();
extern "C" void IntGp();
extern "C" void IntPf();
extern "C" void IntMf();
extern "C" void IntAc();
extern "C" void IntMc();
extern "C" void IntXm();

namespace {
class Entry {
 public:
  static constexpr uint64_t kTypeInterrupt = 0xe;

  void Set(uint16_t selector, void (*handler)(), uint64_t type, uint64_t pl,
           uint64_t ist) {
    selector_ = selector;
    offset_low_ = reinterpret_cast<uint64_t>(handler) & 0xFFFF;
    offset_high_ = reinterpret_cast<uint64_t>(handler) >> 16;
    ist_ = ist;
    type_ = type;
    dpl_ = pl;
    p_ = 1;
  }

 private:
  union {
    uint64_t raw_[2];
    struct {
      uint64_t offset_low_ : 16;
      uint64_t selector_ : 16;
      uint64_t ist_ : 3;
      uint64_t reserved0 : 5;
      uint64_t type_ : 4;
      uint64_t reserved1 : 1;
      uint64_t dpl_ : 2;
      uint64_t p_ : 1;
      uint64_t offset_high_ : 48;
      uint64_t reserved2 : 32;
    } __attribute__((packed));
  };
} __attribute__((aligned(16)));

static_assert(sizeof(Entry) == 16, "idt entry packing issue");

Entry the_idt[256];
}  // namespace

void ebbrt::idt::Load() {
  struct Idtr {
    Idtr(uint16_t size, uint64_t addr) : size(size), addr(addr) {}
    uint16_t size;
    uint64_t addr;
  } __attribute__((packed));

  auto p = Idtr(sizeof(the_idt) - 1, reinterpret_cast<uint64_t>(&the_idt));
  asm volatile("lidt %[idt]" : : [idt] "m"(p));
}

void ebbrt::idt::Init() {
  uint16_t cs;
  asm volatile("mov %%cs, %[cs]" : [cs] "=r"(cs));
  the_idt[0].Set(cs, IntDe, Entry::kTypeInterrupt, 0, 1);
  the_idt[1].Set(cs, IntDb, Entry::kTypeInterrupt, 0, 1);
  the_idt[2].Set(cs, IntNmi, Entry::kTypeInterrupt, 0, 1);
  the_idt[3].Set(cs, IntBp, Entry::kTypeInterrupt, 0, 1);
  the_idt[4].Set(cs, IntOf, Entry::kTypeInterrupt, 0, 1);
  the_idt[5].Set(cs, IntBr, Entry::kTypeInterrupt, 0, 1);
  the_idt[6].Set(cs, IntUd, Entry::kTypeInterrupt, 0, 1);
  the_idt[7].Set(cs, IntNm, Entry::kTypeInterrupt, 0, 1);
  the_idt[8].Set(cs, IntDf, Entry::kTypeInterrupt, 0, 1);
  the_idt[10].Set(cs, IntTs, Entry::kTypeInterrupt, 0, 1);
  the_idt[11].Set(cs, IntNp, Entry::kTypeInterrupt, 0, 1);
  the_idt[12].Set(cs, IntSs, Entry::kTypeInterrupt, 0, 1);
  the_idt[13].Set(cs, IntGp, Entry::kTypeInterrupt, 0, 1);
  the_idt[14].Set(cs, IntPf, Entry::kTypeInterrupt, 0, 1);
  the_idt[16].Set(cs, IntMf, Entry::kTypeInterrupt, 0, 1);
  the_idt[17].Set(cs, IntAc, Entry::kTypeInterrupt, 0, 1);
  the_idt[18].Set(cs, IntMc, Entry::kTypeInterrupt, 0, 1);
  the_idt[19].Set(cs, IntXm, Entry::kTypeInterrupt, 0, 1);

  for (int i = 32; i < 256; ++i) {
    the_idt[i].Set(cs, reinterpret_cast<void (*)()>(EventEntry + (i - 32) * 16),
                   Entry::kTypeInterrupt, 0, 2);
  }
}

extern "C" __attribute__((noreturn)) void ebbrt::idt::EventInterrupt(int num) {
  ebbrt::event_manager->ProcessInterrupt(num);
}

namespace {
void PrintExceptionFrame(ebbrt::idt::ExceptionFrame *ef) {
  ebbrt::kprintf("SS: %#018" PRIx64 " RSP: %#018" PRIx64 "\n", ef->ss, ef->rsp);
  ebbrt::kprintf("FLAGS: %#018" PRIx64 "\n",
                 ef->rflags);  // TODO(Dschatz): print out actual meaning
  ebbrt::kprintf("CS: %#018" PRIx64 " RIP: %#018" PRIx64 "\n", ef->cs, ef->rip);
  ebbrt::kprintf("Error Code: %" PRIx64 "\n", ef->error_code);
  ebbrt::kprintf("RAX: %#018" PRIx64 " RBX: %#018" PRIx64 "\n", ef->rax,
                 ef->rbx);
  ebbrt::kprintf("RCX: %#018" PRIx64 " RDX: %#018" PRIx64 "\n", ef->rcx,
                 ef->rdx);
  ebbrt::kprintf("RSI: %#018" PRIx64 " RDI: %#018" PRIx64 "\n", ef->rsi,
                 ef->rdi);
  ebbrt::kprintf("RBP: %#018" PRIx64 " R8:  %#018" PRIx64 "\n", ef->rbp,
                 ef->r8);
  ebbrt::kprintf("R9:  %#018" PRIx64 " R10: %#018" PRIx64 "\n", ef->r9,
                 ef->r10);
  ebbrt::kprintf("R11: %#018" PRIx64 " R12: %#018" PRIx64 "\n", ef->r11,
                 ef->r12);
  ebbrt::kprintf("R13: %#018" PRIx64 " R14: %#018" PRIx64 "\n", ef->r13,
                 ef->r14);
  ebbrt::kprintf("R15: %#018" PRIx64 "\n", ef->r15);

  // TODO(dschatz): FPU
}
}  // namespace

extern "C" void ebbrt::idt::NmiInterrupt(ExceptionFrame *ef) { kabort(); }

#define UNHANDLED_INTERRUPT(name)                                              \
  extern "C" void ebbrt::idt::name(ExceptionFrame *ef) {                       \
    kprintf("%s\n", __PRETTY_FUNCTION__);                                      \
    PrintExceptionFrame(ef);                                                   \
    kabort();                                                                  \
  }

UNHANDLED_INTERRUPT(DivideErrorException)
UNHANDLED_INTERRUPT(DebugException)
UNHANDLED_INTERRUPT(BreakpointException)
UNHANDLED_INTERRUPT(OverflowException)
UNHANDLED_INTERRUPT(BoundRangeExceededException)
UNHANDLED_INTERRUPT(InvalidOpcodeException)
UNHANDLED_INTERRUPT(DeviceNotAvailableException)
UNHANDLED_INTERRUPT(DoubleFaultException)
UNHANDLED_INTERRUPT(InvalidTssException)
UNHANDLED_INTERRUPT(SegmentNotPresent)
UNHANDLED_INTERRUPT(StackFaultException)
UNHANDLED_INTERRUPT(GeneralProtectionException)
UNHANDLED_INTERRUPT(X87FpuFloatingPointError)
UNHANDLED_INTERRUPT(AlignmentCheckException)
UNHANDLED_INTERRUPT(MachineCheckException)
UNHANDLED_INTERRUPT(SimdFloatingPointException)
