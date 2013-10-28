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
#ifndef EBBRT_LRT_CONFIG_HPP
#define EBBRT_LRT_CONFIG_HPP

#include <cstddef>

#ifdef LRT_BARE
#include <src/lrt/bare/config.hpp>
#elif LRT_ULNX
#include <src/lrt/ulnx/config.hpp>
#endif

#include "lrt/EbbId.hpp"

namespace ebbrt {
  namespace lrt {
    namespace config {
      trans::EbbId 
        find_static_ebb_id(void* config, const char* name);
        uint16_t get_space_id(void* config);
        uint32_t get_static_ebb_count(void* config);
        bool get_multicore(void* config);
    }
  }
}

#endif
