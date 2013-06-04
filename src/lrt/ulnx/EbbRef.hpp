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

#include "ebbrt.hpp"
#include "lrt/EbbId.hpp"
#include "lrt/ulnx/init.hpp"

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

        EbbRef() :
          id_{0}
        {}

        /**
         * @brief EbbRef Object Constructor
         *
         * @param id Given ebb ID
         */
        explicit EbbRef(EbbId id) :
          /* construct the ref corresponding to the given id */
          id_{id}
        {}
        /**
         * @brief Overload arrow operator
         *
         * @return return pointer to ebb reference
         */
        T* operator->() const
        {
          auto it = active_context->local_table_.find(id_);
          if (it == active_context->local_table_.end()) {
            active_context->last_ebbcall_id_ = id_;
            return static_cast<T*>(default_rep);
          } else {
            return static_cast<T*>(it->second);
          }
        }
        /**
         * @brief Conversion operator between EbbRef and EbbID
         *
         * @return EbbId of EbbRef
         */
        operator EbbId() const
        {
          return id_;
        }
      private:
        /**
         * @brief And in the end, the ebb you take is equal to the ebb you make.
         */
        EbbId id_;
      };
    }
  }
}
