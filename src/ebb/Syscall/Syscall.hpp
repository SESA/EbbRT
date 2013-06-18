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
#ifndef EBBRT_EBB_SYSCALL_SYSCALL_HPP
#define EBBRT_EBB_SYSCALL_SYSCALL_HPP

#include <cstdarg>

#include "ebb/ebb.hpp"

namespace ebbrt {
  class Syscall : public EbbRep {
  public:
    static EbbRoot* ConstructRoot();

    virtual int Exit(int val);
    virtual int Execve(char* name, char** argv, char** env);
    virtual int Getpid();
    virtual int Fork();
    virtual int Kill(int pid, int sig);
    virtual int Wait(int* status);
    virtual int Isatty(int fd);
    virtual int Close(int fd);
    virtual int Link(char* path1, char* path2);
    virtual int Lseek(int file, int ptr, int dir);
    virtual int Open(const char* name, int flags, va_list list);
    virtual int Read(int file, char* ptr, int len);
    virtual int Fstat(int file, struct stat* st);
    virtual int Stat(const char* file, struct stat* st);
    virtual int Unlink(char* name);
    virtual int Write(int file, char* ptr, int len);
    virtual int Gettimeofday(struct timeval* p, void* z);
    virtual void* Malloc(size_t size);
    virtual void Free(void* ptr);
    virtual void* Realloc(void* ptr, size_t size);
    virtual void* Calloc(size_t num, size_t size);
  };
  constexpr EbbRef<Syscall> syscall = EbbRef<Syscall>(static_cast<EbbId>(4));
}
#endif
