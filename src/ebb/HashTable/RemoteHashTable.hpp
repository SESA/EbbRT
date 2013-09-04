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
#ifndef EBBRT_EBB_REMOTEHASHTABLE_REMOTEHASHTABLE_HPP
#define EBBRT_EBB_REMOTEHASHTABLE_REMOTEHASHTABLE_HPP

#include <functional>

#include "ebb/ebb.hpp"
#include "ebb/EventManager/Future.hpp"

namespace ebbrt {
  class RemoteHashTable : public EbbRep {
  public:
    class TimeoutError : public std::runtime_error
    {
    public:
      TimeoutError() : std::runtime_error{"Timeout while fetching data"} {}
    };

    virtual Future<Buffer>
    Get(NetworkId from, const std::string& key) = 0;

    virtual void
    Set(const std::string& key, Buffer val) = 0;
  protected:
    RemoteHashTable(EbbId id) : EbbRep{id} {}
  };
}
#endif
