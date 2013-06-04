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
#ifndef EBBRT_ARCH_X86_64_PIT_HPP
#define EBBRT_ARCH_X86_64_PIT_HPP

#include "arch/io.hpp"

namespace {
  const uint16_t PIT_CHANNEL_0 = 0x40;
  const uint16_t PIT_COMMAND = 0x43;
};

namespace ebbrt {
  namespace pit {
    /**
     * @brief Disable programmable interval timer
     */
    inline void
    disable() {
      //set the counter to 0
      out8(PIT_CHANNEL_0, 0x00);
      //affecting channel 0, write to both hi/lo bytes, operate in mode
      //0, binary
      out8(PIT_COMMAND, 0x30);
    }
  }
}
#endif
