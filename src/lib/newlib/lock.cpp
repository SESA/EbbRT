/*
 * Copyright (C) 2012 by Project SESA, Boston University
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
    access_once(mut->lock) = 0;
    return 0;
  }

  if (mut->loc == ebbrt::lrt::event::get_location()) {
    //this is a recursive lock by us
    if (mut->count == INT16_MAX) {
      return 1; //holding the lock with too much recursion!
    }
    mut->count++;
    __sync_synchronize();
    access_once(mut->lock) = 0;
    return 0;
  }

  //someone holds the lock that is not us, fail!
  __sync_synchronize();
  access_once(mut->lock) = 0;
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
  access_once(mut->lock) = 0;
}
