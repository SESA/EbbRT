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

#include "lrt/event.hpp"
#include "sync/compiler.hpp"

//DO NOT CHANGE: these are defined in the newlib tree
typedef void *_LOCK_T;
typedef void *_LOCK_RECURSIVE_T;

extern "C" void
ebbrt_newlib_lock_init_recursive(_LOCK_RECURSIVE_T *lock)
{
  *lock = 0;
}

extern "C" void
ebbrt_newlib_lock_close_recursive(_LOCK_RECURSIVE_T *lock)
{
  return;
}

extern "C" void
ebbrt_newlib_lock_acquire(_LOCK_T *lock)
{
  while (!__sync_bool_compare_and_swap(lock, 0, 1))
    ;
}

typedef union {
  intptr_t val;
  struct {
    int16_t count;
    uint8_t loc; //location
    int8_t lock;
  };
} rec_spinlock;

static_assert(sizeof(rec_spinlock) == sizeof(void *), "rec_spinlock size");

extern "C" int
ebbrt_newlib_lock_try_acquire_recursive(_LOCK_RECURSIVE_T *lock)
{
  rec_spinlock *mut = reinterpret_cast<rec_spinlock *>(lock);
  while (!__sync_bool_compare_and_swap(&mut->lock, 0, 1))
    ;

  if (mut->count == 0) {
    //we are the first to lock it
    mut->loc = ebbrt::lrt::event::get_location();
    mut->count = 1;
    __sync_synchronize();
    ebbrt::access_once(mut->lock) = 0;
    return 0;
  }

  if (mut->loc == ebbrt::lrt::event::get_location()) {
    //this is a recursive lock by us
    if (mut->count == INT16_MAX) {
      return 1; //holding the lock with too much recursion!
    }
    mut->count++;
    __sync_synchronize();
    ebbrt::access_once(mut->lock) = 0;
    return 0;
  }

  //someone holds the lock that is not us, fail!
  __sync_synchronize();
  ebbrt::access_once(mut->lock) = 0;
  return 1;
}

extern "C" void
ebbrt_newlib_lock_acquire_recursive(_LOCK_RECURSIVE_T *lock)
{
  while (ebbrt_newlib_lock_try_acquire_recursive(lock) != 0)
    ;
}

extern "C" void
ebbrt_newlib_lock_release(_LOCK_T *lock)
{
  __sync_synchronize();
  *lock = 0;
}

extern "C" void
ebbrt_newlib_lock_release_recursive(_LOCK_RECURSIVE_T *lock)
{
  rec_spinlock *mut = reinterpret_cast<rec_spinlock *>(lock);
  while (!__sync_bool_compare_and_swap(&mut->lock, 0, 1))
    ;

  mut->count--;

  __sync_synchronize();
  ebbrt::access_once(mut->lock) = 0;
}
