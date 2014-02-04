//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>

#include <ebbrt/Debug.h>
#include <ebbrt/GeneralPurposeAllocator.h>
#include <ebbrt/Gthread.h>
#include <ebbrt/VMem.h>

#include <ebbrt/Cpu.h>
#include <atomic>

extern "C" int ebbrt_newlib_exit(int val) {
  UNIMPLEMENTED();
  return 0;
}

extern "C" int ebbrt_newlib_execve(char* name, char** argv, char** env) {
  UNIMPLEMENTED();
  return 0;
}

extern "C" int ebbrt_newlib_getpid(void) {
  UNIMPLEMENTED();
  return 0;
}

extern "C" int ebbrt_newlib_fork(void) {
  UNIMPLEMENTED();
  return 0;
}

extern "C" int ebbrt_newlib_kill(int pid, int sig) {
  UNIMPLEMENTED();
  return 0;
}

extern "C" int ebbrt_newlib_wait(int* status) {
  UNIMPLEMENTED();
  return 0;
}

extern "C" int ebbrt_newlib_isatty(int fd) {
  UNIMPLEMENTED();
  return 0;
}

extern "C" int ebbrt_newlib_close(int file) {
  UNIMPLEMENTED();
  return 0;
}

extern "C" int ebbrt_newlib_link(char* path1, char* path2) {
  UNIMPLEMENTED();
  return 0;
}

extern "C" int ebbrt_newlib_lseek(int file, int ptr, int dir) {
  UNIMPLEMENTED();
  return 0;
}

extern "C" int ebbrt_newlib_open(const char* name, int flags, va_list list) {
  UNIMPLEMENTED();
  return 0;
}

extern "C" int ebbrt_newlib_read(int file, char* ptr, int len) {
  UNIMPLEMENTED();
  return 0;
}

extern "C" int ebbrt_newlib_fstat(int file, struct stat* st) {
  UNIMPLEMENTED();
  return 0;
}

extern "C" int ebbrt_newlib_stat(const char* file, struct stat* st) {
  UNIMPLEMENTED();
  return 0;
}

extern "C" int ebbrt_newlib_unlink(char* name) {
  UNIMPLEMENTED();
  return 0;
}

extern "C" int ebbrt_newlib_write(int file, char* ptr, int len) {
  UNIMPLEMENTED();
  return 0;
}

extern "C" void* ebbrt_newlib_malloc(size_t size) {
  return ebbrt::gp_allocator->Alloc(size);
}

extern "C" void ebbrt_newlib_free(void* ptr) { ebbrt::gp_allocator->Free(ptr); }

extern "C" void* ebbrt_newlib_realloc(void*, size_t) {
  UNIMPLEMENTED();
  return nullptr;
}

extern "C" void* ebbrt_newlib_calloc(size_t num, size_t size) {
  auto p = ebbrt_newlib_malloc(num * size);
  if (p == nullptr)
    return p;

  std::memset(p, 0, num * size);
  return p;
}

extern "C" void* ebbrt_newlib_memalign(size_t alignment, size_t size) {
  size = std::max(size, alignment);
  auto ptr = ebbrt::gp_allocator->Alloc(size);
  ebbrt::kbugon(ebbrt::align::Down(ptr, alignment) != ptr,
                "Memalign failed to allocate aligned region of memory");
  return ptr;
}

extern "C" caddr_t ebbrt_newlib_sbrk(int incr) {
  UNIMPLEMENTED();
  return 0;
}

extern "C" int ebbrt_newlib_gettimeofday(struct timeval* p, void* z) {
  UNIMPLEMENTED();
  return 0;
}

struct RLock {
  static const constexpr uint32_t kNoOwner = -1;
  uint32_t owner;
  uint32_t count;
};

typedef void *_LOCK_T;
typedef void *_LOCK_RECURSIVE_T;
#define STATIC_ASSERT(COND,MSG) typedef char static_assertion_##MSG[(COND)?1:-1]

STATIC_ASSERT((sizeof(struct RLock)==sizeof(void *)), sizeof_RLock_not_sizeof_void_ptr);
STATIC_ASSERT(ebbrt::Cpu::kMaxCpus < ((uint64_t)(1ULL<<32)), uint32_not_enough_for_max_cpus);

extern "C" void ebbrt_newlib_lock_init_recursive(_LOCK_RECURSIVE_T *lock) {
  RLock *l = (struct RLock *)lock;
  l->owner = RLock::kNoOwner;
  l->count = 0;
}

extern "C" void ebbrt_newlib_lock_close_recursive(_LOCK_RECURSIVE_T* lock) {
  UNIMPLEMENTED();
}

extern "C" void ebbrt_newlib_lock_acquire(_LOCK_T* lock) { UNIMPLEMENTED(); }

extern "C" int
ebbrt_newlib_lock_try_acquire_recursive(_LOCK_RECURSIVE_T* lock) {
  UNIMPLEMENTED();
  return 0;
}

extern "C" void ebbrt_newlib_lock_acquire_recursive(_LOCK_RECURSIVE_T* lock) {
  auto glock =
      static_cast<__gthread_recursive_mutex_t*>(static_cast<void*>(lock));
  auto ret = ebbrt_gthread_recursive_mutex_lock(glock);
  ebbrt::kbugon(ret != 0, "lock acquisition failed!\n");
}

extern "C" void ebbrt_newlib_lock_release(_LOCK_RECURSIVE_T* lock) {
  UNIMPLEMENTED();
}

extern "C" void ebbrt_newlib_lock_release_recursive(_LOCK_RECURSIVE_T* lock) {
  auto glock =
      static_cast<__gthread_recursive_mutex_t*>(static_cast<void*>(lock));
  auto ret = ebbrt_gthread_recursive_mutex_unlock(glock);
  ebbrt::kbugon(ret != 0, "unlock failed!\n");
}
