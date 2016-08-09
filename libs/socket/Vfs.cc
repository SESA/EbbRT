//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//

#include "Vfs.h"
#include <ebbrt/Debug.h>

int ebbrt::Vfs::RegisterFd(ebbrt::EbbRef<ebbrt::Vfs::Fd> ref) {
  auto fd = fd_++;
  descriptor_map_.emplace(fd, std::move(ref));
  return fd;
}

ebbrt::EbbRef<ebbrt::Vfs::Fd> ebbrt::Vfs::Lookup(int fd) {
  auto it = descriptor_map_.find(fd);
  if (it == descriptor_map_.end()) {
    throw std::invalid_argument("Failed to locate file descriptor");
  }
  return it->second;
}
