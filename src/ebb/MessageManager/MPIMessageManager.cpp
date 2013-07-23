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
#include <cstdint>
#include <cstdlib>

#include "ebb/SharedRoot.hpp"
#include "ebb/EventManager/EventManager.hpp"
#include "ebb/MessageManager/MPIMessageManager.hpp"

ebbrt::EbbRoot*
ebbrt::MPIMessageManager::ConstructRoot()
{
  return new SharedRoot<MPIMessageManager>;
}

namespace {
  class MessageHeader {
  public:
    ebbrt::EbbId ebb;
  };

  constexpr int MESSAGE_MANAGER_TAG = 0x8812;
}

ebbrt::MPIMessageManager::MPIMessageManager()
{
}


void
ebbrt::MPIMessageManager::Send(NetworkId to,
                            EbbId id,
                            BufferList buffers,
                            std::function<void()> cb)
{
  size_t len = sizeof(MessageHeader);
  for (const auto& buffer : buffers) {
    len += buffer.second;
  }
  char* aggregate = new char[len];

  reinterpret_cast<MessageHeader*>(aggregate)->ebb = id;
  char* location = aggregate + sizeof(MessageHeader);
  for (const auto& buffer : buffers) {
    memcpy(location, buffer.first, buffer.second);
    location += buffer.second;
  }

  if (MPI_Send(aggregate, len, MPI_CHAR, to.rank, MESSAGE_MANAGER_TAG,
               MPI_COMM_WORLD) != MPI_SUCCESS) {
    throw std::runtime_error("MPI_Send failed");
  }

  assert(!cb);
}

void
ebbrt::MPIMessageManager::StartListening()
{
  interrupt_ = event_manager->AllocateInterrupt([]() {
      auto me = static_cast<EbbRef<MPIMessageManager> >(message_manager);
      me->DispatchMessage();
    });

  event_manager->RegisterFunction([]() {
      auto me = static_cast<EbbRef<MPIMessageManager> >(message_manager);
      return me->CheckForInterrupt();
    });
}

int
ebbrt::MPIMessageManager::CheckForInterrupt()
{
  int flag;
  if (MPI_Iprobe(MPI_ANY_SOURCE, MESSAGE_MANAGER_TAG, MPI_COMM_WORLD,
                 &flag, &status_) != MPI_SUCCESS) {
    throw std::runtime_error("Iprobe failed");
  }

  if (flag) {
    return interrupt_;
  } else {
    return -1;
  }
}

void
ebbrt::MPIMessageManager::DispatchMessage()
{
  int len;
  if (MPI_Get_count(&status_, MPI_CHAR, &len) != MPI_SUCCESS) {
    throw std::runtime_error("MPI_Get_count failed");
  }
  uint8_t* buf = new uint8_t[len];
  MPI_Status status;
  if (MPI_Recv(buf, len, MPI_CHAR, MPI_ANY_SOURCE, MESSAGE_MANAGER_TAG,
               MPI_COMM_WORLD, &status) != MPI_SUCCESS) {
    throw std::runtime_error("MPI_Recv failed");
  }

  auto mh = reinterpret_cast<const MessageHeader*>(buf);
  EbbRef<EbbRep> ebb {mh->ebb};
  ebb->HandleMessage(buf + sizeof(MessageHeader),
                     len - sizeof(MessageHeader));
  delete buf;
}
