/*
  EbbRT: Distributed, Elastic, Runtime
  Copyright (C) 2013 SESA Group, Boston University

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef EBBRT_ARCH_X86_64_RTC_HPP
#define EBBRT_ARCH_X86_64_RTC_HPP

#include "arch/io.hpp"

namespace {
  const uint16_t CMOS_SELECT = 0x70;
  const uint16_t CMOS_REGISTER = 0x71;

  /* CMOS 0Bh - RTC - STATUS REGISTER B (read/write) */
  /* Bitfields for Real-Time Clock status register B: */
  /* Bit(s) Description (Table C002) */
  /* 7 enable cycle update */
  /* 6 enable periodic interrupt */
  /* 5 enable alarm interrupt */
  /* 4 enable update-ended interrupt */
  /* 3 enable square wave output */
  /* 2 Data Mode - 0: BCD, 1: Binary */
  /* 1 24/12 hour selection - 1 enables 24 hour mode */
  /* 0 Daylight Savings Enable - 1 enables */
  const uint8_t CMOS_STATUS_B = 0xb;
  const uint8_t CMOS_STATUS_B_INT_UPDATE_ENDED = 1 << 4;
  const uint8_t CMOS_STATUS_B_INT_ALARM = 1 << 5;
  const uint8_t CMOS_STATUS_B_INT_PERIODIC = 1 << 6;

  const uint8_t CMOS_STATUS_C = 0xc;
};

namespace ebbrt {
  namespace rtc {
    /**
     * @brief Disable real-time clock
     */
    inline void
    disable() {
      out8(CMOS_SELECT, CMOS_STATUS_B);
      uint8_t status_b = in8(CMOS_REGISTER);
      //Mask off the three interrupts
      status_b &= ~(CMOS_STATUS_B_INT_UPDATE_ENDED |
                    CMOS_STATUS_B_INT_ALARM |
                    CMOS_STATUS_B_INT_PERIODIC);
      out8(CMOS_SELECT, CMOS_STATUS_B);
      out8(CMOS_REGISTER, status_b);

      //We read register C to clear out pending interrupts
      out8(CMOS_SELECT, CMOS_STATUS_C);
    }
  }
}
#endif
