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
#error "Don't include this file directly"
#endif

#include <cstdlib>
#include <cstddef>
#include "lrt/bare/arch/trans.hpp"

namespace ebbrt {
  namespace lrt {
    namespace trans {
      class EbbRep;
      /**
       * @brief Local translation table entry.  By default the ref of an
       * "empty" entry is pointed to the `shadow` virtual function table
       * which, in turn, will act as the objects virtual function table and
       * call into the corresponding default function that initiates a  miss.
       *
       * For configured entries, the ref is referenced (method is
       * resolved using the objects own
       * v-table), and the `shadow` table is unused.
       */
      class LocalEntry {
      public:
        /* Ebb ref */
        EbbRep* ref;
        /* `shadow` virtual function table*/
        void (*(*rep)[256])(); //pointer to a size 256 array of
                               //pointers to functions taking no
                               //arguments and having no return
      };

      /** virtual memory trick ***/
      LocalEntry* const local_table =
        reinterpret_cast<LocalEntry*>(LOCAL_MEM_VIRT_BEGIN);

      /**
       * Construct early roots, before exceptions and global
       * constructors.
       *
       * @return early_init_count
       */
      int early_init_ebbs();
      /**
       * @brief Construct roots for statically linked Ebbs
       * int early_init_count
       */
      void init_ebbs(int early_init_count);
      /**
       * @brief Per-core initialization
       */
      void init_cpu();
    }
  }
}
