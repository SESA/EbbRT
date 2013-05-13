#ifndef EBBRT_ARCH_X86_64_IDTDESC_HPP
#define EBBRT_ARCH_X86_64_IDTDESC_HPP

#include <cstdint>

#include "sync/compiler.hpp"

namespace ebbrt {
  class IdtDesc {
  public:
    enum SegType {
      INTERRUPT = 0xe,
      TRAP = 0xf
    };
    void set(uint16_t sel, void *addr,
             SegType t=INTERRUPT, int pl=0, int st=0) {
      offset_low = reinterpret_cast<uint64_t>(addr) & 0xFFFF;
      offset_high = reinterpret_cast<uint64_t>(addr) >> 16;
      selector = sel;
      ist = st;
      type = t;
      dpl = pl;
      p = 1;
    }
  private:
    union {
      uint64_t raw[2];
      struct {
        uint64_t offset_low :16;
        uint64_t selector :16;
        uint64_t ist :3;
        uint64_t :5;
        uint64_t type :4;
        uint64_t :1;
        uint64_t dpl :2;
        uint64_t p :1;
        uint64_t offset_high :48;
        uint64_t :32;
      } __attribute__((packed));
    };
  };
  static_assert(sizeof(IdtDesc) == 16, "packing issue");
}


#endif
