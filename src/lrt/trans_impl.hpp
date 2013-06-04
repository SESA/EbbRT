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
#ifndef EBBRT_LRT_TRANS_IMPL_HPP
#define EBBRT_LRT_TRANS_IMPL_HPP

#ifdef LRT_BARE
#include <src/lrt/bare/trans_impl.hpp>
#endif

#include <cstddef>
#include <unordered_map>

#include "lrt/InitRoot.hpp"
#include "lrt/trans.hpp"

namespace ebbrt {
  namespace lrt {
    namespace trans {
      /** virtual function table */
      extern void (*default_vtable[256])();

      /**
       * @brief Set up and process a miss on the local translation system.
       * This function is upcalled from assembly, while running on the
       * alternative stack.
       *
       * @param args Arguments of the `missed` ebb call
       * @param fnum Function number of call
       * @param fret Pointer to return value of call
       *
       * @return
       */
      extern "C" bool _trans_precall(Args* args,
                                     ptrdiff_t fnum,
                                     FuncRet* fret);

      /**
       * @brief Local miss handle post call. Upcalled from assembly
       *
       * @param ret
       *
       * @return
       */
      extern "C" void* _trans_postcall(void* ret);

      class EbbRep;
      /**
       * @brief Cache ebb rep in the local translation system. This is called
       * most likely called from the ebb manager.
       *
       * @param id Ebb's id
       * @param rep Location of location ebb representative
       */
      void cache_rep(EbbId id, EbbRep* rep);

      class EbbRoot;
      /**
       * @brief Swap out existing miss handler
       *
       * @param root
       */
      void install_miss_handler(EbbRoot* root);

      class RootBinding {
      public:
        ebbrt::lrt::trans::EbbId id;
        ebbrt::lrt::trans::EbbRoot* root;
      };

      const RootBinding& initial_root_table(unsigned i);
      extern InitRoot init_root;
    }
  }
}
#endif
