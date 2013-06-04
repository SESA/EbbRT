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
