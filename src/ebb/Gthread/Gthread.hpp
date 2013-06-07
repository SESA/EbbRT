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
#ifndef EBBRT_EBB_GTHREAD_GTHREAD_HPP
#define EBBRT_EBB_GTHREAD_GTHREAD_HPP

#include "ebb/ebb.hpp"

namespace ebbrt {
  class Gthread : public EbbRep {
  public:
    static EbbRoot* ConstructRoot();

    //DO NOT CHANGE: these are defined in the gcc and newlib trees to match
    typedef void* Key;
    typedef void* Once;
    typedef void* Mutex;
    typedef void* RecursiveMutex;

    virtual void MutexInit(Mutex* mutex);
    virtual void RecursiveMutexInit(RecursiveMutex* mutex);
    virtual int Active();
    virtual int DoOnce(Once* once, void (*func) (void));
    virtual int KeyCreate(Key* keyp, void (*dtor) (void *));
    virtual int KeyDelete(Key key);
    virtual void* GetSpecific(Key key);
    virtual int SetSpecific(Key key, const void* ptr);
    virtual int MutexDestroy(Mutex* mutex);
    virtual int MutexLock(Mutex* mutex);
    virtual int MutexTryLock(Mutex* mutex);
    virtual int MutexUnlock(Mutex* mutex);
    virtual int RecursiveMutexDestroy(RecursiveMutex* mutex);
    virtual int RecursiveMutexLock(RecursiveMutex* mutex);
    virtual int RecursiveMutexTryLock(RecursiveMutex* mutex);
    virtual int RecursiveMutexUnlock(RecursiveMutex* mutex);
  };
  extern bool gthread_active;
  constexpr EbbRef<Gthread> gthread = EbbRef<Gthread>(static_cast<EbbId>(3));
}
#endif
