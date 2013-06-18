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
#include <cstring>
#include <cstdint>
#include <cerrno>

#include "ebb/Gthread/Gthread.hpp"

using namespace ebbrt;

extern "C" void
ebbrt_gthread_mutex_init(Gthread::Mutex* mutex)
{
  gthread->MutexInit(mutex);
}

extern "C" void
ebbrt_gthread_recursive_mutex_init(Gthread::RecursiveMutex* mutex)
{
  gthread->RecursiveMutexInit(mutex);
}

extern "C" int
ebbrt_gthread_active_p(void)
{
  return gthread_active;
}

extern "C" int
ebbrt_gthread_once(Gthread::Once* once, void (*func) (void))
{
  return gthread->DoOnce(once, func);
}

extern "C" int
ebbrt_gthread_key_create(Gthread::Key* keyp,
                         void (*dtor) (void*))
{
  return gthread->KeyCreate(keyp, dtor);
}

extern "C" int
ebbrt_gthread_key_delete(Gthread::Key key)
{
  return gthread->KeyDelete(key);
}

extern "C" void*
ebbrt_gthread_getspecific(Gthread::Key key)
{
  return gthread->GetSpecific(key);
}

extern "C" int
ebbrt_gthread_setspecific(Gthread::Key key, const void* ptr)
{
  return gthread->SetSpecific(key, ptr);
}

extern "C" int
ebbrt_gthread_mutex_destroy(Gthread::Mutex* mutex)
{
  return gthread->MutexDestroy(mutex);
}

extern "C" int
ebbrt_gthread_mutex_lock(Gthread::Mutex* mutex)
{
  return gthread->MutexLock(mutex);
}

extern "C" int
ebbrt_gthread_mutex_trylock(Gthread::Mutex* mutex)
{
  return gthread->MutexTryLock(mutex);
}

extern "C" int
ebbrt_gthread_mutex_unlock(Gthread::Mutex* mutex)
{
  return gthread->MutexUnlock(mutex);
}

extern "C" int
ebbrt_gthread_recursive_mutex_trylock(Gthread::RecursiveMutex* mutex)
{
  return gthread->RecursiveMutexTryLock(mutex);
}

extern "C" int
ebbrt_gthread_recursive_mutex_lock(Gthread::RecursiveMutex* mutex)
{
  return gthread->RecursiveMutexLock(mutex);
}

extern "C" int
ebbrt_gthread_recursive_mutex_unlock(Gthread::RecursiveMutex* mutex)
{
  return gthread->RecursiveMutexUnlock(mutex);
}
