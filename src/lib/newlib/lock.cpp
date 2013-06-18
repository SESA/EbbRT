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
#include <cstdint>

#include "ebb/Gthread/Gthread.hpp"

using namespace ebbrt;
extern "C" int
ebbrt_gthread_active_p(void);

extern "C" void
ebbrt_newlib_lock_init_recursive(Gthread::RecursiveMutex* mutex)
{
  if (ebbrt_gthread_active_p()) {
    gthread->RecursiveMutexInit(mutex);
  }
}

extern "C" void
ebbrt_newlib_lock_close_recursive(Gthread::RecursiveMutex* mutex)
{
  if (ebbrt_gthread_active_p()) {
    gthread->RecursiveMutexDestroy(mutex);
  }
}

extern "C" void
ebbrt_newlib_lock_acquire(Gthread::Mutex* mutex)
{
  if (ebbrt_gthread_active_p()) {
    gthread->MutexLock(mutex);
  }
}

extern "C" int
ebbrt_newlib_lock_try_acquire_recursive(Gthread::RecursiveMutex* mutex)
{
  if (ebbrt_gthread_active_p()) {
    return gthread->RecursiveMutexTryLock(mutex);
  }
  return 0;
}

extern "C" void
ebbrt_newlib_lock_acquire_recursive(Gthread::RecursiveMutex* mutex)
{
  if (ebbrt_gthread_active_p()) {
    gthread->RecursiveMutexLock(mutex);
  }
}

extern "C" void
ebbrt_newlib_lock_release(Gthread::Mutex* mutex)
{
  if (ebbrt_gthread_active_p()) {
    gthread->MutexLock(mutex);
  }
}

extern "C" void
ebbrt_newlib_lock_release_recursive(Gthread::RecursiveMutex* mutex)
{
  if (ebbrt_gthread_active_p()) {
    gthread->RecursiveMutexUnlock(mutex);
  }
}
