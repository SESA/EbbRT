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
#ifndef EBBRT_LRT_EBBREP_HPP
#define EBBRT_LRT_EBBREP_HPP

#ifdef __linux__
#include <cassert>
#elif __ebbrt__
#include "lrt/bare/assert.hpp"
#endif

#include "misc/network.hpp"

namespace ebbrt {
  namespace lrt {
    namespace trans {
      /**
       * @brief Ebb Representative
       */
      class EbbRep {
      public:
        virtual void HandleMessage(NetworkId from,
                                   const char* buf,
                                   size_t len)
        {
#ifdef __linux__
          assert(0);
#elif __ebbrt__
          LRT_ASSERT(0);
#endif
        }
      };
    }
  }
}

#endif
