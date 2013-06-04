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
#ifndef EBBRT_LRT_BARE_BOOT_HPP
#define EBBRT_LRT_BARE_BOOT_HPP

#include "lrt/bare/arch/boot.hpp"

namespace ebbrt {
  namespace lrt {
    namespace boot {
      /**
       *  Initialize LRT.
       *  This is done once, before core-specific initialization.
       */
      void init() __attribute__((noreturn));
      /**
       * Start other cores then do per core initialization.
       * @param[in] num_cores The total number of cores to start
       */
      void init_smp(unsigned num_cores) __attribute__((noreturn));
      /**
       * Per core initialization.
       */
      void init_cpu();
    }
  }
}

#endif
