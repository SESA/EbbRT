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
#ifndef EBBRT_EBB_DISTRIBUTEDROOT_HPP
#define EBBRT_EBB_DISTRIBUTEDROOT_HPP

#include <map>

#include "ebb/ebb.hpp"
#include "ebb/EbbManager/EbbManager.hpp"
#include "sync/spinlock.hpp"

namespace ebbrt {
  template <typename T>
    /**
     * @brief Distributed Ebb Root
     * A representative is constructed for each location
     */
    class DistributedRoot : public EbbRoot {
      public:
        /**
         * @brief Handle the construction of the rep. This gets called on a
         * miss to construct the local representative and update the
         * translation system.
         *
         * @param args Ebb call arguments
         * @param fnum Ebb call function number
         * @param fret Return value address
         * @param id   Ebb Id
         *
         * @return
         */
        bool PreCall(Args* args, ptrdiff_t fnum,
            lrt::trans::FuncRet* fret, EbbId id) override
        {
          T* ref;
          /* check local map of representative exists. This is in the case a rep
           * entry is expelled from our local cache */
          lock_.Lock();
          auto it = reps_.find(get_location());
          if (it == reps_.end()) {
            /* no rep for this location */
            /** the following ordering is nessessary due to the fact that the
             * new call may trigger a miss on the memory manager */
            ref = new T();
            /* cache representative in translation system */
            ebb_manager->CacheRep(id, ref);
            reps_[get_location()] = ref;
          } else {
            /* rep found in map */
            ref = it->second;
            ebb_manager->CacheRep(id, ref);
          }
          lock_.Unlock();
          //
          *reinterpret_cast<EbbRep**>(args) = ref;
          // rep is a pointer to pointer to array 256 of pointer to
          // function returning void
          void (*(**rep)[256])() = reinterpret_cast<void (*(**)[256])()>(ref);
          /* set return function to be the method of ebb call */
          fret->func = (**rep)[fnum];
          return true;
        }
        /**
         * @brief Ebb construction post-call.
         *
         * @param ret ebb calls return
         *
         * @return Return ebb call return value
         */
        void* PostCall(void* ret) override
        {
          return ret;
        }
      private:
        /**
         * @brief local mapping of the distributed representative of root
         */
        std::map<Location, T*> reps_;
        Spinlock lock_;
    };
}

#endif
