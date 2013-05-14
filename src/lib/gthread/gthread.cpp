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

#include <cstring>
#include <cstdint>
#include <cerrno>

#include "lrt/event.hpp"
#include "sync/compiler.hpp"

//DO NOT CHANGE: these are defined in the gcc tree to match
//FIXME: maybe these should be padded?
typedef void *__gthread_key_t;
typedef void *__gthread_once_t;
typedef void *__gthread_mutex_t;
typedef void *__gthread_recursive_mutex_t;

namespace {
  const int NUM_KEYS = 32;
  int key_index = 0;
  const void *key_array[NUM_KEYS];
};

extern "C" void
ebbrt_gthread_mutex_init(__gthread_mutex_t *mutex)
{
  *mutex = 0;
}

extern "C" void
ebbrt_gthread_recursive_mutex_init(__gthread_recursive_mutex_t *mutex)
{
  *mutex = 0;
}

extern "C" int
ebbrt_gthread_active_p(void)
{
  return 1;
}

extern "C" int
ebbrt_gthread_once(__gthread_once_t *once, void (*func) (void))
{
  __gthread_once_t val = __sync_val_compare_and_swap(once, 0, 1);
  if (val == 0) {
    func();
    __sync_synchronize();
    *once = reinterpret_cast<__gthread_once_t>(2);
  } else {
    while (ebbrt::access_once(*once) != reinterpret_cast<__gthread_once_t>(2))
      ;
  }
  return 0;
}

extern "C" int
ebbrt_gthread_key_create(__gthread_key_t *keyp,
                         void (*dtor) (void *) __attribute__((unused)))
{
  int index = __sync_fetch_and_add(&key_index, 1);
  if (index >= NUM_KEYS) {
    return ENOMEM;
  }
  const void ***key_pointer =
    const_cast<const void ***>(reinterpret_cast<void ***>(keyp));
  *key_pointer = &key_array[index];
  **key_pointer = NULL;
  return 0;
}

extern "C" int
ebbrt_gthread_key_delete(__gthread_key_t key)
{
  return 0;
}

extern "C" void *
ebbrt_gthread_getspecific(__gthread_key_t key)
{
  return (*reinterpret_cast<void **>(key));
}

extern "C" int
ebbrt_gthread_setspecific(__gthread_key_t key, const void *ptr)
{
  *const_cast<const void **>(reinterpret_cast<void **>(key)) = ptr;
  return 0;
}

extern "C" int
ebbrt_gthread_mutex_destroy(__gthread_mutex_t *mutex __attribute__((unused)))
{
  return 0;
}

extern "C" int
ebbrt_gthread_mutex_lock(__gthread_mutex_t *mutex)
{
  while (!__sync_bool_compare_and_swap(mutex, 0, 1))
    ;
  return 0;
}

extern "C" int
ebbrt_gthread_mutex_trylock(__gthread_mutex_t *mutex)
{
  if (__sync_bool_compare_and_swap(mutex, 0, 1)) {
    return 0;
  }
  return EBUSY;
}

extern "C" int
ebbrt_gthread_mutex_unlock(__gthread_mutex_t *mutex)
{
  __sync_synchronize();
  *mutex = 0;
  return 0;
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
ebbrt_gthread_recursive_mutex_trylock(__gthread_recursive_mutex_t *mutex)
{
  rec_spinlock *mut = reinterpret_cast<rec_spinlock *>(mutex);
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

extern "C" int
ebbrt_gthread_recursive_mutex_lock(__gthread_recursive_mutex_t *mutex)
{
  while (ebbrt_gthread_recursive_mutex_trylock(mutex) != 0)
    ;

  return 0;
}

extern "C" int
ebbrt_gthread_recursive_mutex_unlock(__gthread_recursive_mutex_t *mutex)
{
  rec_spinlock *mut = reinterpret_cast<rec_spinlock *>(mutex);
  while (!__sync_bool_compare_and_swap(&mut->lock, 0, 1))
    ;

  mut->count--;

  __sync_synchronize();
  ebbrt::access_once(mut->lock) = 0;
  return 0;
}
