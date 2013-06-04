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
