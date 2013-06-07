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
#include "ebb/Gthread/Gthread.hpp"
#include "lrt/bare/assert.hpp"

using namespace ebbrt;

bool ebbrt::gthread_active = false;

EbbRoot*
Gthread::ConstructRoot()
{
  return new SharedRoot<Gthread>();
  gthread_active = true;
}

void
Gthread::MutexInit(Mutex* mutex)
{
  LRT_ASSERT(0);
}

void
Gthread::RecursiveMutexInit(RecursiveMutex* rec_mutex)
{
  LRT_ASSERT(0);
}

int
Gthread::Active()
{
  LRT_ASSERT(0);
  return 0;
}

int
Gthread::DoOnce(Once* once, void (*func) (void))
{
  LRT_ASSERT(0);
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
  LRT_ASSERT(0);
  return 0;
}

int
Gthread::MutexTryLock(Mutex* mutex)
{
  LRT_ASSERT(0);
  return 0;
}

int
Gthread::MutexUnlock(Mutex* mutex)
{
  LRT_ASSERT(0);
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
