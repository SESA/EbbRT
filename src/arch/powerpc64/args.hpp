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
#ifndef EBBRT_ARCH_ARGS_HPP
#error "Don't include this file directly"
#endif

#include <cstdint>

#include "lrt/trans.hpp"

namespace ebbrt {
  class Args {
  public:
    inline uint64_t& this_pointer() {
#ifdef __linux__
      if (gprs[0] == reinterpret_cast<uintptr_t>(lrt::trans::default_rep)) {
        return gprs[0];
      }
      return gprs[1];
#elif __ebbrt__
      if (gprs[0] > lrt::trans::LOCAL_MEM_VIRT_BEGIN &&
          gprs[0] < lrt::trans::LOCAL_MEM_VIRT_END) {
        return gprs[0];
      }
      return gprs[1];
#else
#error "Unsupported Platform"
#endif
    }
    uint64_t* gprs;
    uint64_t* fprs;
    uint64_t* stack_args;
  };
}
