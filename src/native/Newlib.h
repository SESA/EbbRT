//          Copyright Boston University SESA Group 2016
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_NEWLIB_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_NEWLIB_H_

#include <stdarg.h>
#include <stddef.h>

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

extern int ebbrt_newlib_exit(int);
extern int ebbrt_newlib_execve(char *, char **, char **);
extern int ebbrt_newlib_getpid(void);
extern int ebbrt_newlib_fork(void);
extern int ebbrt_newlib_kill(int, int);
extern int ebbrt_newlib_wait(int *);
extern int ebbrt_newlib_isatty(int);
extern int ebbrt_newlib_close(int);
extern int ebbrt_newlib_link(char *, char *);
extern int ebbrt_newlib_lseek(int, int, int);
extern int ebbrt_newlib_open(const char *, int, va_list);
extern int ebbrt_newlib_read(int, char *, int);
extern int ebbrt_newlib_fstat(int, struct stat *);
extern int ebbrt_newlib_stat(const char *, struct stat *);
extern int ebbrt_newlib_unlink(char *);
extern int ebbrt_newlib_write(int, char *, int);
extern void* ebbrt_newlib_malloc(size_t);
extern void ebbrt_newlib_free(void*);
extern void* ebbrt_newlib_realloc(void*, size_t);
extern void* ebbrt_newlib_calloc(size_t, size_t);
extern void* ebbrt_newlib_memalign(size_t, size_t);
extern int ebbrt_newlib_gettimeofday(struct timeval *, void *);

#ifdef __cplusplus
}
#endif
#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_NEWLIB_H_
