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
#include "lrt/bare/assert.hpp"

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
