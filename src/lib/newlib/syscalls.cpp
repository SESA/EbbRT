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
#include "ebb/Syscall/Syscall.hpp"

extern "C" int
ebbrt_newlib_exit(int val)
{
  return ebbrt::syscall->Exit(val);
}

extern "C" int
ebbrt_newlib_execve(char *name, char **argv, char **env)
{
  return ebbrt::syscall->Execve(name, argv, env);
}

extern "C" int
ebbrt_newlib_getpid(void)
{
  return ebbrt::syscall->Getpid();
}

extern "C" int
ebbrt_newlib_fork(void)
{
  return ebbrt::syscall->Fork();
}

extern "C" int
ebbrt_newlib_kill(int pid, int sig)
{
  return ebbrt::syscall->Kill(pid, sig);
}

extern "C" int
ebbrt_newlib_wait(int *status)
{
  return ebbrt::syscall->Wait(status);
}

extern "C" int
ebbrt_newlib_isatty(int fd)
{
  return ebbrt::syscall->Isatty(fd);
}

extern "C" int
ebbrt_newlib_close(int file)
{
  return ebbrt::syscall->Close(file);
}

extern "C" int
ebbrt_newlib_link(char *path1, char *path2)
{
  return ebbrt::syscall->Link(path1, path2);
}

extern "C" int
ebbrt_newlib_lseek(int file, int ptr, int dir)
{
  return ebbrt::syscall->Lseek(file, ptr, dir);
}

extern "C" int
ebbrt_newlib_open(const char *name, int flags, va_list list)
{
  return ebbrt::syscall->Open(name, flags, list);
}

extern "C" int
ebbrt_newlib_read(int file, char *ptr, int len)
{
  return ebbrt::syscall->Read(file, ptr, len);
}

extern "C" int
ebbrt_newlib_fstat(int file, struct stat *st)
{
  return ebbrt::syscall->Fstat(file, st);
}

extern "C" int
ebbrt_newlib_stat(const char *file, struct stat *st)
{
  return ebbrt::syscall->Stat(file, st);
}

extern "C" int
ebbrt_newlib_unlink(char *name)
{
  return ebbrt::syscall->Unlink(name);
}

extern "C" int
ebbrt_newlib_write(int file, char *ptr, int len)
{
  return ebbrt::syscall->Write(file, ptr, len);
}

extern "C" int
ebbrt_newlib_gettimeofday(struct timeval *p, void *z)
{
  return ebbrt::syscall->Gettimeofday(p, z);
}

extern "C" void* ebbrt_newlib_malloc(size_t size)
{
  return ebbrt::memory_allocator->Malloc(size);
}

extern "C" void ebbrt_newlib_free(void* ptr)
{
  return ebbrt::memory_allocator->Free(ptr);
}

extern "C" void* ebbrt_newlib_realloc(void* ptr, size_t size)
{
  return ebbrt::memory_allocator->Realloc(ptr, size);
}

extern "C" void* ebbrt_newlib_calloc(size_t num, size_t size)
{
  return ebbrt::memory_allocator->Calloc(num, size);
}
