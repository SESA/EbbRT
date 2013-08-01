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
#ifndef EBBRT_EBB_EBBMANAGER_EBBMANAGER_HPP
#define EBBRT_EBB_EBBMANAGER_EBBMANAGER_HPP

#include "ebb/ebb.hpp"
#include "lrt/trans_impl.hpp"
#include "lrt/InitRoot.hpp"

namespace ebbrt {
  class EbbManager : public EbbRep {
  public:
    virtual void CacheRep(EbbId id, EbbRep* rep) = 0;
    virtual EbbId AllocateId() = 0;
    virtual void Bind(EbbRoot* (*factory)(), EbbId id) = 0;
    virtual ~EbbManager() {}
  protected:
    EbbManager(EbbId id) : EbbRep{id} {}
  private:
    friend lrt::trans::InitRoot;
    virtual void Install() = 0;
  };
  constexpr EbbRef<EbbManager> ebb_manager =
    EbbRef<EbbManager>(static_cast<EbbId>(2));
}

#endif
