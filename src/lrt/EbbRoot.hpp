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
#ifndef EBBRT_LRT_EBBROOT_HPP
#define EBBRT_LRT_EBBROOT_HPP

namespace ebbrt {
  class Args;
  namespace lrt {
    namespace trans {
      class FuncRet;
      /**
       * @brief Ebb Root
       */
      class EbbRoot {
      public:
        /**
         * @brief Initial stage of ebb call
         *
         * @param args Ebbcall arguments
         * @param fnum
         * @param fret
         * @param id
         *
         * @return
         */
        virtual bool PreCall(Args* args, ptrdiff_t fnum,
                             FuncRet* fret, EbbId id) = 0;
        /**
         * @brief Final stage of ebb call
         s
         * @param ret
         *
         * @return
         */
        virtual void* PostCall(void* ret) = 0;

        virtual ~EbbRoot() {}
      };
    }
  }
}
#endif
