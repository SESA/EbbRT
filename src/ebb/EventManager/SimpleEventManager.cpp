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
#include "ebb/SharedRoot.hpp"
#include "ebb/EventManager/SimpleEventManager.hpp"
#ifdef __ebbrt__
#include "lrt/bare/assert.hpp"
#endif

ebbrt::EbbRoot*
ebbrt::SimpleEventManager::ConstructRoot()
{
  return new SharedRoot<SimpleEventManager>();
}

ebbrt::SimpleEventManager::SimpleEventManager() : next_{32}
{
}

#include <iostream>

uint8_t
ebbrt::SimpleEventManager::AllocateInterrupt(std::function<void()> func)
{
  lock_.Lock();
  uint8_t ret = next_++;
  map_.insert(std::make_pair(ret, std::move(func)));
  lock_.Unlock();
  return ret;
}

void
ebbrt::SimpleEventManager::HandleInterrupt(uint8_t interrupt)
{
  lock_.Lock();
#ifdef __linux__
  assert(map_.find(interrupt) != map_.end());
#elif __ebbrt__
  LRT_ASSERT(map_.find(interrupt) != map_.end());
#endif
  const auto& f = map_[interrupt];
  lock_.Unlock();
  if (f) {
    f();
  }
}
