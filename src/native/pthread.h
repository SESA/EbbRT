#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_PTHREAD_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_PTHREAD_H_

#include "sched.h"
#include <ebbrt/Debug.h>
#include <ebbrt/EventManager.h>
#include <ebbrt/SpinLock.h>

extern "C" {
typedef unsigned long pthread_t;
typedef unsigned int pthread_key_t;
typedef struct {
  int is_initialized;
  void* stackaddr;
  int stacksize;
  int contentionscope;
  int inheritsched;
  int schedpolicy;
  sched_param schedparam;
  int detachstate;
} pthread_attr_t;

typedef struct {
} pthread_barrier_t;
typedef struct {
} pthread_barrierattr_t;
typedef struct {
} pthread_cond_t;
typedef struct {
} pthread_condattr_t;
typedef struct {
} pthread_mutexattr_t;
typedef struct {
} pthread_rwlock_t;
typedef struct {
} pthread_rwlockattr_t;
typedef struct {
} pthread_spinlock_t;

typedef struct { ebbrt::SpinLock spinlock; } pthread_mutex_t;

void pthread_attr_init(pthread_attr_t* attr);
int pthread_cond_broadcast(pthread_cond_t* cond);
int pthread_cond_signal(pthread_cond_t* cond);
int pthread_cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex,
                           const struct timespec* abstime);
int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
int pthread_cond_destroy(pthread_cond_t* cond);
int pthread_cond_init(pthread_cond_t* __restrict cond,
                      const pthread_condattr_t* __restrict attr);

pthread_t pthread_self();
int pthread_create(pthread_t*, pthread_attr_t*, void* (*)(void*), void*);
int pthread_join(pthread_t, void**);
int pthread_detach(pthread_t thread);
int pthread_equal(pthread_t t1, pthread_t t2);

void* pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void* value);
int pthread_key_create(pthread_key_t* key, void (*destructor)(void*));
int pthread_mutexattr_destroy(pthread_mutexattr_t* attr);
int pthread_mutexattr_init(pthread_mutexattr_t* attr);
int pthread_mutexattr_gettype(const pthread_mutexattr_t* __restrict attr,
                              int* __restrict type);
int pthread_mutexattr_settype(pthread_mutexattr_t* attr, int type);

typedef struct {
} pthread_once_t;
int pthread_once(pthread_once_t* once_control, void (*init_routine)(void));

#define PTHREAD_MUTEX_INITIALIZER                                              \
  { 0 }
#define PTHREAD_COND_INITIALIZER                                               \
  {}
#define PTHREAD_ONCE_INIT                                                      \
  {}

#define PTHREAD_MUTEX_RECURSIVE 33

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_trylock(pthread_mutex_t* mutex);
int pthread_mutex_unlock(pthread_mutex_t* mutex);
int pthread_mutex_destroy(pthread_mutex_t* mutex);

}  // "C"

#endif
