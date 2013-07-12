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
#ifdef __bg__
#include <mpi.h>
#endif

#include "ebb/SharedRoot.hpp"
#include "ebb/Console/RemoteConsole.hpp"
#include "ebb/MessageManager/MessageManager.hpp"

#ifdef __ebbrt__
#include "lrt/bare/assert.hpp"
#endif

#ifdef __linux__
#include <iostream>
#endif
ebbrt::EbbRoot*
ebbrt::RemoteConsole::ConstructRoot()
{
  return new SharedRoot<RemoteConsole>;
}

void
ebbrt::RemoteConsole::Write(const char* str,
                            std::function<void()> cb)
{
#ifdef __linux__
#ifndef __bg__
  std::cout << str;
  if (cb) {
    cb();
  }
#else
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    std::cout << str;
  } else {
    BufferList list = BufferList(1, std::make_pair(str, strlen(str) + 1));
    NetworkId id;
    id.rank = 0;
    message_manager->Send(id, console, std::move(list));
  }
#endif
#elif __ebbrt__
  BufferList list = BufferList(1, std::make_pair(str, strlen(str) + 1));
  LRT_ASSERT(!cb);
  NetworkId id;
  id.mac_addr[0] = 0xff;
  id.mac_addr[1] = 0xff;
  id.mac_addr[2] = 0xff;
  id.mac_addr[3] = 0xff;
  id.mac_addr[4] = 0xff;
  id.mac_addr[5] = 0xff;
  message_manager->Send(id, console, std::move(list));
#endif
}

void
ebbrt::RemoteConsole::HandleMessage(const uint8_t* msg,
                                    size_t len)
{
#ifdef __linux__
#ifndef __bg__
  std::cout << msg;
#else
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    std::cout << msg;
  } else {
    assert(0);
  }
#endif
#elif __ebbrt__
  LRT_ASSERT(0);
#endif
}
