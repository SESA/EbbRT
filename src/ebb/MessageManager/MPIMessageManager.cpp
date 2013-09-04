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

#include "app/app.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/EventManager/EventManager.hpp"
#include "ebb/MessageManager/MPIMessageManager.hpp"

ebbrt::EbbRoot*
ebbrt::MPIMessageManager::ConstructRoot()
{
  return new SharedRoot<MPIMessageManager>;
}

// registers symbol for configuration
__attribute__((constructor(65535)))
static void _reg_symbol()
{
  ebbrt::app::AddSymbol("MessageManager",
                        ebbrt::MPIMessageManager::ConstructRoot);
}

namespace {
  class MessageHeader {
  public:
    ebbrt::EbbId ebb;
  };

  constexpr int MESSAGE_MANAGER_TAG = 0x8812;
}

ebbrt::MPIMessageManager::MPIMessageManager(EbbId id) : MessageManager{id}
{
}

ebbrt::Buffer
ebbrt::MPIMessageManager::Alloc(size_t size)
{
  size += sizeof(MessageHeader);
  auto mem = std::malloc(size);
  if (mem == nullptr) {
    throw std::bad_alloc();
  }
  return (Buffer(mem, size) + sizeof(MessageHeader));
}

void
ebbrt::MPIMessageManager::Send(NetworkId to,
                               EbbId id,
                               Buffer buffer)
{
  auto buf = buffer - sizeof(MessageHeader);
  auto header = reinterpret_cast<MessageHeader*>(buf.data());
  header->ebb = id;

  MPI_Request req;
  if (MPI_Isend(buf.data(), buf.length(), MPI_CHAR,
                to.rank, MESSAGE_MANAGER_TAG, MPI_COMM_WORLD,
                &req) != MPI_SUCCESS) {
    throw std::runtime_error("MPI_Isend failed");
  }
  reqs_.push_back(std::move(req));
  bufs_.push_back(std::move(buf));
}

void
ebbrt::MPIMessageManager::StartListening()
{
  interrupt_ = event_manager->AllocateInterrupt([]() {
      EbbRef<MPIMessageManager> me = message_manager;
      me->DispatchMessage();
    });

  event_manager->RegisterFunction([]() {
      EbbRef<MPIMessageManager> me = message_manager;
      return me->CheckForInterrupt();
    });
}

int
ebbrt::MPIMessageManager::CheckForInterrupt()
{
  if (!reqs_.empty()) {
    int index;
    int flag;
    MPI_Status status;
    do {
      if (MPI_Testany(reqs_.size(), reqs_.data(), &index,
                      &flag, &status) != MPI_SUCCESS) {
        throw std::runtime_error("Testany failed");
      }
      if (flag) {
        reqs_.erase(reqs_.begin() + index);
        bufs_.erase(bufs_.begin() + index);
      }
    } while (flag && !reqs_.empty());
  }

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

  auto mem = std::malloc(len);
  if (mem == nullptr) {
    throw std::bad_alloc();
  }

  MPI_Status status;
  if (MPI_Recv(mem, len, MPI_CHAR, MPI_ANY_SOURCE, MESSAGE_MANAGER_TAG,
               MPI_COMM_WORLD, &status) != MPI_SUCCESS) {
    throw std::runtime_error("MPI_Recv failed");
  }

  auto buf = Buffer{mem, static_cast<size_t>(len)};

  auto mh = reinterpret_cast<const MessageHeader*>(buf.data());
  EbbRef<EbbRep> ebb {mh->ebb};

  NetworkId from;
  from.rank = status.MPI_SOURCE;

  ebb->HandleMessage(from,
                     buf + sizeof(MessageHeader));
}
