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
#ifndef EBBRT_EBB_EVENTMANAGER_SIMPLEEVENTMANAGER_HPP
#define EBBRT_EBB_EVENTMANAGER_SIMPLEEVENTMANAGER_HPP

#include <unordered_map>

#include "ebb/EventManager/EventManager.hpp"
#include "sync/spinlock.hpp"

namespace ebbrt {
  class SimpleEventManager : public EventManager {
  public:
    static EbbRoot* ConstructRoot();

    SimpleEventManager();
    uint8_t AllocateInterrupt(std::function<void()> func) override;
  private:
    void HandleInterrupt(uint8_t interrupt) override;

    std::unordered_map<uint8_t, std::function<void()> > map_;
    uint8_t next_;
    Spinlock lock_;
  };
}

#endif
