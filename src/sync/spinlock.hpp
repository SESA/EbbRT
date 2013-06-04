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
#ifndef EBBRT_SYNC_SPINLOCK_HPP
#define EBBRT_SYNC_SPINLOCK_HPP

#include <atomic>

namespace ebbrt {
  class Spinlock {
  public:
    Spinlock() : lock_{ATOMIC_FLAG_INIT} {}
    inline void Lock()
    {
      while (lock_.test_and_set(std::memory_order_acquire))
        ;
    }
    inline bool TryLock()
    {
      return !lock_.test_and_set(std::memory_order_acquire);
    }
    inline void Unlock()
    {
      lock_.clear(std::memory_order_release);
    }
  private:
    std::atomic_flag lock_;
  };
}

#endif
