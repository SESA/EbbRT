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
#ifndef EBBRT_LRT_TRANS_HPP
#define EBBRT_LRT_TRANS_HPP

#include <cstddef>

#ifdef LRT_BARE
#include <src/lrt/bare/trans.hpp>
#elif LRT_ULNX
#include <src/lrt/ulnx/trans.hpp>
#endif

#include "lrt/EbbId.hpp"

namespace ebbrt {
  namespace lrt {
    namespace trans {
      class FuncRet {
      public:
        union {
          void (*func)();
          void* ret;
        };
      };
    }
  }
}

#endif
