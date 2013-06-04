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
