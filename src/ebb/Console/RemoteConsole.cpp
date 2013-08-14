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

#include <memory>

#include "app/app.hpp"
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

// registers symbol for configuration
__attribute__((constructor(65535)))
static void _reg_symbol()
{
  ebbrt::app::AddSymbol ("Console", ebbrt::RemoteConsole::ConstructRoot);
}

ebbrt::RemoteConsole::RemoteConsole(EbbId id) : Console(id) {}

ebbrt::Buffer
ebbrt::RemoteConsole::Alloc(size_t size)
{
#ifdef __linux__
#ifndef __bg__
  auto mem = std::malloc(size);
  if (mem == nullptr) {
    throw std::bad_alloc();
  }
  return Buffer{mem, size};
#else
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    auto mem = std::malloc(size);
    if (mem == nullptr) {
      throw std::bad_alloc();
    }
    return Buffer{mem, size};
  } else {
    return message_manager->Alloc(size);
  }
#endif
#elif __ebbrt__
  return message_manager->Alloc(size);
#endif
}

void
ebbrt::RemoteConsole::Write(Buffer buffer)
{
#ifdef __linux__
#ifndef __bg__
  std::cout << buffer.data();
#else
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    std::cout << buffer.data();
  } else {
    NetworkId id;
    id.rank = 0;
    message_manager->Send(id, ebbid_, std::move(buffer));
  }
#endif
#elif __ebbrt__
  NetworkId id;
  id.mac_addr[0] = 0xff;
  id.mac_addr[1] = 0xff;
  id.mac_addr[2] = 0xff;
  id.mac_addr[3] = 0xff;
  id.mac_addr[4] = 0xff;
  id.mac_addr[5] = 0xff;
  message_manager->Send(id, ebbid_, std::move(buffer));
#endif
}

void
ebbrt::RemoteConsole::HandleMessage(NetworkId from,
                                    Buffer buffer)
{
#ifdef __linux__
#ifndef __bg__
  std::cout << buffer.data();
#else
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    std::cout << buffer.data();
  } else {
    assert(0);
  }
#endif
#elif __ebbrt__
  LRT_ASSERT(0);
#endif
}
