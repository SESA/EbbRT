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
#include "ebb/SharedRoot.hpp"
#include "ebb/Syscall/Syscall.hpp"
#include "lrt/bare/assert.hpp"
#include "src/lrt/config.hpp"

#ifdef __ebbrt__
ADD_EARLY_CONFIG_SYMBOL(Syscall, &ebbrt::Syscall::ConstructRoot)
#endif

using namespace ebbrt;

EbbRoot*
Syscall::ConstructRoot()
{
  return new SharedRoot<Syscall>();
}

int
Syscall::Exit(int val)
{
  LRT_ASSERT(0);
  return 0;
}

int
Syscall::Execve(char* name, char** argv, char** env)
{
  LRT_ASSERT(0);
  return 0;
}

int
Syscall::Getpid()
{
  LRT_ASSERT(0);
  return 0;
}

int
Syscall::Fork()
{
  LRT_ASSERT(0);
  return 0;
}

int
Syscall::Kill(int pid, int sig)
{
  LRT_ASSERT(0);
  return 0;
}

int
Syscall::Wait(int* status)
{
  LRT_ASSERT(0);
  return 0;
}

int
Syscall::Isatty(int fd)
{
  LRT_ASSERT(0);
  return 0;
}

int
Syscall::Close(int fd)
{
  LRT_ASSERT(0);
  return 0;
}

int
Syscall::Link(char* path1, char* path2)
{
  LRT_ASSERT(0);
  return 0;
}

int
Syscall::Lseek(int file, int ptr, int dir)
{
  LRT_ASSERT(0);
  return 0;
}

int
Syscall::Open(const char* name, int flags, va_list list)
{
  LRT_ASSERT(0);
  return 0;
}

int
Syscall::Read(int file, char* ptr, int len)
{
  LRT_ASSERT(0);
  return 0;
}

int
Syscall::Fstat(int file, struct stat* st)
{
  LRT_ASSERT(0);
  return 0;
}

int
Syscall::Stat(const char* file, struct stat* st)
{
  LRT_ASSERT(0);
  return 0;
}

int
Syscall::Unlink(char* name)
{
  LRT_ASSERT(0);
  return 0;
}

int
Syscall::Write(int file, char* ptr, int len)
{
  LRT_ASSERT(0);
  return 0;
}

int
Syscall::Gettimeofday(struct timeval* p, void* z)
{
  LRT_ASSERT(0);
  return 0;
}

void*
Syscall::Malloc(size_t size)
{
  LRT_ASSERT(0);
  return nullptr;
}

void
Syscall::Free(void* ptr)
{
  LRT_ASSERT(0);
}

void*
Syscall::Realloc(void* ptr, size_t size)
{
  LRT_ASSERT(0);
  return nullptr;
}

void*
Syscall::Calloc(size_t num, size_t size)
{
  LRT_ASSERT(0);
  return nullptr;
}
