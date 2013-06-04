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
          ref_{reinterpret_cast<T**>(&(local_table[0].ref))}
        {}

        /**
         * @brief EbbRef Object Constructor
         *
         * @param id Given ebb ID
         */
        constexpr explicit EbbRef(EbbId id) :
          /* construct the ref corresponding to the given id */
          ref_{reinterpret_cast<T**>(&(local_table[id].ref))}
        {}
        /**
         * @brief Overload arrow operator
         *
         * @return return pointer to ebb reference
         */
        T* operator->() const
        {
          return *ref_;
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
         * @brief And in the end, the ebb you take is equal to the ebb you make.
         */
        T** ref_;
      };
    }
  }
}
