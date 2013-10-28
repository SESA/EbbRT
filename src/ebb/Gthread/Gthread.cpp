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
#include <atomic>

#include "ebb/SharedRoot.hpp"
#include "ebb/Gthread/Gthread.hpp"
#include "lrt/bare/assert.hpp"
#include "src/lrt/config.hpp"

using namespace ebbrt;

bool ebbrt::gthread_active = false;

#ifdef __ebbrt__
ADD_EARLY_CONFIG_SYMBOL(Gthread, &ebbrt::Gthread::ConstructRoot)
#endif

EbbRoot*
Gthread::ConstructRoot()
{
  return new SharedRoot<Gthread>();
  gthread_active = true;
}

void
Gthread::MutexInit(Mutex* mutex)
{
  auto flag = reinterpret_cast<std::atomic_flag*>(mutex);
  flag->clear();
}

void
Gthread::RecursiveMutexInit(RecursiveMutex* rec_mutex)
{
  LRT_ASSERT(0);
}

int
Gthread::DoOnce(Once* once, void (*func) (void))
{
  auto flag = reinterpret_cast<std::atomic<int>*>(once);

  int expected = 0;
  auto success = atomic_compare_exchange_strong(flag, &expected, 1);

  if (success) {
    func();
    *flag = 2;
  } else {
    while (*flag != 2)
      ;
  }
  return 0;
}

int
Gthread::KeyCreate(Key* keyp, void (*dtor) (void *))
{
  LRT_ASSERT(0);
  return 0;
}

int
Gthread::KeyDelete(Key key)
{
  LRT_ASSERT(0);
  return 0;
}

void*
Gthread::GetSpecific(Key key)
{
  LRT_ASSERT(0);
  return nullptr;
}

int
Gthread::SetSpecific(Key key, const void* ptr)
{
  LRT_ASSERT(0);
  return 0;
}

int
Gthread::MutexDestroy(Mutex* mutex)
{
  LRT_ASSERT(0);
  return 0;
}

int
Gthread::MutexLock(Mutex* mutex)
{
  auto flag = reinterpret_cast<std::atomic_flag*>(mutex);
  while (flag->test_and_set(std::memory_order_acquire))
    ;
  return 0;
}

int
Gthread::MutexTryLock(Mutex* mutex)
{
  auto flag = reinterpret_cast<std::atomic_flag*>(mutex);
  return !flag->test_and_set(std::memory_order_acquire);
}

int
Gthread::MutexUnlock(Mutex* mutex)
{
  auto flag = reinterpret_cast<std::atomic_flag*>(mutex);
  flag->clear(std::memory_order_release);
  return 0;
}

int
Gthread::RecursiveMutexDestroy(RecursiveMutex* mutex)
{
  LRT_ASSERT(0);
  return 0;
}

int
Gthread::RecursiveMutexLock(RecursiveMutex* mutex)
{
  LRT_ASSERT(0);
  return 0;
}

int
Gthread::RecursiveMutexTryLock(RecursiveMutex* mutex)
{
  LRT_ASSERT(0);
  return 0;
}

int
Gthread::RecursiveMutexUnlock(RecursiveMutex* mutex)
{
  LRT_ASSERT(0);
  return 0;
}
