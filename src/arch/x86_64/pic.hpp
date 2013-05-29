#ifndef EBBRT_ARCH_X86_64_PIC_HPP
#define EBBRT_ARCH_X86_64_PIC_HPP

#include "arch/io.hpp"

namespace ebbrt {
  namespace pic {
    static const uint16_t MASTER_COMMAND = 0x20;
    static const uint16_t MASTER_DATA = 0x21;
    static const uint16_t SLAVE_COMMAND = 0xa0;
    static const uint16_t SLAVE_DATA = 0xa1;

    static const uint8_t ICW1_ICW4 = 0x01;
    static const uint8_t ICW1_INIT = 0x10;

    static const uint8_t ICW4_8086 = 0x01;

    static inline void
    disable()
    {
      out8(ICW1_ICW4 + ICW1_INIT, MASTER_COMMAND);
      out8(ICW1_ICW4 + ICW1_INIT, SLAVE_COMMAND);

      out8(0x10, MASTER_DATA);
      out8(0x18, SLAVE_DATA);

      out8(1 << 2, MASTER_DATA);
      out8(2, SLAVE_DATA);

      out8(ICW4_8086, MASTER_DATA);
      out8(ICW4_8086, SLAVE_DATA);

      //Disable the pic by masking all irqs
      //OCW 4 to a pic with a0 set (hence the +1 addr) will mask irqs
      //We set all bits to mask all the irqs
      out8(0xff, MASTER_DATA);
      out8(0xff, SLAVE_DATA);
    }
  }
}
#endif
