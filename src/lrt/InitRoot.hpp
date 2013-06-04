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
#ifndef EBBRT_LRT_INITROOT_HPP
#define EBBRT_LRT_INITROOT_HPP

#include "lrt/EbbRoot.hpp"

namespace ebbrt {
  namespace lrt {
    namespace trans {
      /**
       * @brief InitRoot acts as our initial root miss handler / translation
       * ebb that works off the initial root table constructed on bring up.
       * InitRoot will be replaced by the event manager upon the miss on an
       * ebb not in the static root table.
       */
      class InitRoot : public EbbRoot {
        bool PreCall(Args* args, ptrdiff_t fnum,
                     FuncRet* fret, EbbId id) override;
        void* PostCall(void* ret) override;
      };
    }
  }
}
#endif
