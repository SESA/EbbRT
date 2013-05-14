#ifndef EBBRT_ARCH_X86_64_IDTR_HPP
#define EBBRT_ARCH_X86_64_IDTR_HPP

namespace ebbrt {
  class Idtr {
  public:
    uint16_t limit;
    uint64_t base;
  } __attribute__((packed));


  inline void
  lidt(const Idtr& idtr)
  {
    asm volatile (
                  "lidt %[idtr]"
                  : //no output
                  : [idtr] "m" (idtr),
                    "m" (*reinterpret_cast<ebbrt::IdtDesc*>(idtr.base))
                  );
  }
}
#endif
