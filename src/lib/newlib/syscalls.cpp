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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>

#include "ebb/MemoryAllocator/MemoryAllocator.hpp"
#include "lrt/assert.hpp"

#define NYI \
  LRT_ASSERT(0);


extern "C" int
ebbrt_newlib_exit(int val)
{
  NYI;
  return 0;
}

extern "C" int
ebbrt_newlib_execve(char *name, char **argv, char **env)
{
  NYI;
  return 0;
}

extern "C" int
ebbrt_newlib_getpid(void)
{
  NYI;
  return 0;
}

extern "C" int
ebbrt_newlib_fork(void)
{
  NYI;
  return 0;
}

extern "C" int
ebbrt_newlib_kill(int pid, int sig)
{
  NYI;
  return 0;
}

extern "C" int
ebbrt_newlib_wait(int *status)
{
  NYI;
  return 0;
}

extern "C" int
ebbrt_newlib_isatty(int fd)
{
  NYI
  return 0;
}

extern "C" int
ebbrt_newlib_close(int file)
{
  NYI;
  return 0;
}

extern "C" int
ebbrt_newlib_link(char *path1, char *path2)
{
  NYI;
  return 0;
}

extern "C" int
ebbrt_newlib_lseek(int file, int ptr, int dir)
{
  NYI;
  return 0;
}

extern "C" int
ebbrt_newlib_open(const char *name, int flags, va_list list)
{
  NYI;
  return 0;
}

extern "C" int
ebbrt_newlib_read(int file, char *ptr, int len)
{
  NYI;
  return 0;
}

extern "C" int
ebbrt_newlib_fstat(int file, struct stat *st)
{
  NYI;
  return 0;
}

extern "C" int
ebbrt_newlib_stat(const char *file, struct stat *st)
{
  NYI;
  return 0;
}

extern "C" int
ebbrt_newlib_unlink(char *name)
{
  NYI;
  return 0;
}

extern "C" int
ebbrt_newlib_write(int file, char *ptr, int len)
{
  NYI;
  return 0;
}

extern "C" caddr_t
ebbrt_newlib_sbrk(int incr)
{
  NYI;
  return 0;
}

extern "C" int
ebbrt_newlib_gettimeofday(struct timeval *p, void *z)
{
  NYI;
  return 0;
}

extern "C" void* ebbrt_newlib_malloc(size_t size)
{
  return ebbrt::memory_allocator->malloc(size);
}

extern "C" void ebbrt_newlib_free(void* ptr)
{
  ebbrt::memory_allocator->free(ptr);
}

extern "C" void* ebbrt_newlib_realloc(void* ptr, size_t size)
{
  return ebbrt::memory_allocator->realloc(ptr, size);
}

extern "C" void* ebbrt_newlib_calloc(size_t num, size_t size)
{
  return ebbrt::memory_allocator->calloc(num, size);
}
