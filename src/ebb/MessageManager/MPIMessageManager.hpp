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
#ifndef EBBRT_EBB_MESSAGEMANAGER_MPIMESSAGEMANAGER_HPP
#define EBBRT_EBB_MESSAGEMANAGER_MPIMESSAGEMANAGER_HPP

#include "ebb/MessageManager/MessageManager.hpp"

#include <mpi.h>

namespace ebbrt {
  class MPIMessageManager : public MessageManager {
  public:
    static EbbRoot* ConstructRoot();
    MPIMessageManager(EbbId id);
    virtual void Send(NetworkId to,
                      EbbId ebb,
                      BufferList buffers,
                      std::function<void()> cb = nullptr) override;
    virtual void StartListening() override;
  private:
    virtual int CheckForInterrupt();
    virtual void DispatchMessage();

    uint8_t interrupt_;
    MPI_Status status_;
    std::vector<MPI_Request> reqs_;
    std::vector<std::vector<char> > bufs_;
  };
}
#endif
