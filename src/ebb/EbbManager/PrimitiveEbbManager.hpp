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
#ifndef EBBRT_EBB_EBBMANAGER_PRIMITIVEEBBMANAGER_HPP
#define EBBRT_EBB_EBBMANAGER_PRIMITIVEEBBMANAGER_HPP

#include <map>
#include <unordered_map>

#include "ebb/EbbManager/EbbManager.hpp"
#include "sync/spinlock.hpp"

namespace ebbrt {
  class PrimitiveEbbManager : public EbbManager {
  public:
    PrimitiveEbbManager(EbbId id,
                        std::unordered_map<EbbId, EbbRoot*>& root_table,
                        Spinlock& root_table_lock,
                        std::unordered_map<EbbId, EbbRoot* (*)()>& factory_table,
                        Spinlock& factory_table_lock);
    void CacheRep(EbbId id, EbbRep* rep) override;
    EbbId AllocateId() override;
    void Bind(EbbRoot* (*factory)(), EbbId id) override;
  private:
    virtual void Install() override;
    friend class PrimitiveEbbManagerRoot;
    EbbId next_free_;
    std::unordered_map<EbbId, EbbRoot*>& root_table_;
    Spinlock& root_table_lock_;
    std::unordered_map<EbbId, EbbRoot* (*)()>& factory_table_;
    Spinlock& factory_table_lock_;
  };

  EbbRoot* PrimitiveEbbManagerConstructRoot();
  class PrimitiveEbbManagerRoot : public EbbRoot {
  public:
    bool PreCall(Args* args, ptrdiff_t fnum,
                 lrt::trans::FuncRet* fret, EbbId id) override;
    void* PostCall(void* ret) override;
  private:
    std::map<Location, PrimitiveEbbManager*> reps_;
    Spinlock lock_;
  };
}

#endif
