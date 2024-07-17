//          Copyright Boston University SESA Group 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_NEWLIB_LOCK_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_NEWLIB_LOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void* _LOCK_T;
typedef void* _LOCK_RECURSIVE_T;

extern void ebbrt_newlib_lock_init(_LOCK_T*);
extern void ebbrt_newlib_lock_init_recursive(_LOCK_RECURSIVE_T*);
extern void ebbrt_newlib_lock_close(_LOCK_T*);
extern void ebbrt_newlib_lock_close_recursive(_LOCK_RECURSIVE_T*);
extern void ebbrt_newlib_lock_acquire(_LOCK_T*);
extern void ebbrt_newlib_lock_acquire_recursive(_LOCK_RECURSIVE_T*);
extern int ebbrt_newlib_lock_try_acquire(_LOCK_T*);
extern int ebbrt_newlib_lock_try_acquire_recursive(_LOCK_RECURSIVE_T*);
extern void ebbrt_newlib_lock_release(_LOCK_T*);
extern void ebbrt_newlib_lock_release_recursive(_LOCK_RECURSIVE_T*);
#ifdef __cplusplus
}
#endif
#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_NEWLIB_LOCK_H_
