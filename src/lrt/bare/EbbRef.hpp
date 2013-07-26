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
#ifndef EBBRT_LRT_EBBREF_HPP
#error "Don't include this file directly"
#endif

#include "lrt/trans.hpp"

namespace ebbrt {
  namespace lrt {
    namespace trans {
      /**
       * @brief Elastic Building Block template
       */
      template <class T>
      class EbbRef {
      public:
        /**
         * @brief Default EbbRef Constructor
         *
         * The default constructor points to id 0 which should become
         * the NullEbb
         */

        constexpr EbbRef() :
          ref_{LOCAL_MEM_VIRT_BEGIN}
        {}

        /**
         * @brief EbbRef Object Constructor
         *
         * @param id Given ebb ID
         */
        constexpr explicit EbbRef(EbbId id) :
          /* construct the ref corresponding to the given id */
          ref_{LOCAL_MEM_VIRT_BEGIN + sizeof(LocalEntry) * id}
        {}
        /**
         * @brief Overload arrow operator
         *
         * @return return pointer to ebb reference
         */
        T* operator->() const
        {
          return *reinterpret_cast<T**>(ref_);
        }
        /**
         * @brief Conversion operator between EbbRef and EbbID
         *
         * @return EbbId of EbbRef
         */
        constexpr operator EbbId() const
        {
          return reinterpret_cast<LocalEntry*>(ref_)
            - local_table;
        }
      private:
        /**
         * Ref holds the pointer to the local table entry.
         * It is stored as a uintptr_t to allow us to have constexpr
         * constructors
         */
        uintptr_t ref_;
      };
    }
  }
}
