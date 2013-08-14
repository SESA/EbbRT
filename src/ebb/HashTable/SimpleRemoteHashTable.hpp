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
#ifndef EBBRT_EBB_SIMPLEREMOTEHASHTABLE_SIMPLEREMOTEHASHTABLE_HPP
#define EBBRT_EBB_SIMPLEREMOTEHASHTABLE_SIMPLEREMOTEHASHTABLE_HPP

#include "ebb/HashTable/RemoteHashTable.hpp"

namespace ebbrt {
  class SimpleRemoteHashTable : public RemoteHashTable {
  public:
    static EbbRoot* ConstructRoot();

    SimpleRemoteHashTable(EbbId id);

    virtual Future<Buffer>
    Get(NetworkId from, const std::string& key) override;

    virtual void
    Set(const std::string& key, Buffer val) override;

    virtual void
    HandleMessage(NetworkId from,
                  Buffer buffer);
  private:
    enum HTOp {
      GET_REQUEST,
      GET_RESPONSE
    };

    struct Header {
      HTOp op;
      unsigned op_id;
    };

    void HandleGetRequest(NetworkId from, unsigned op_id, Buffer buffer);
    void HandleGetResponse(NetworkId from, unsigned op_id, Buffer buffer);

    std::unordered_map<std::string, Buffer> table_;
    std::unordered_map<unsigned, Promise<Buffer> > promise_map_;
    std::atomic_uint op_id_;
  };
}
#endif
