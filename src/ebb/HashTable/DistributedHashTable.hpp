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
#ifndef EBBRT_EBB_HASHTABLE_DISTRIBUTEDHASHTABLE_HPP
#define EBBRT_EBB_HASHTABLE_DISTRIBUTEDHASHTABLE_HPP

#include <map>

#include "ebb/HashTable/HashTable.hpp"

namespace ebbrt {
  class DistributedHashTable : public HashTable {
  public:
    static EbbRoot* ConstructRoot();

    DistributedHashTable(EbbId id);

    virtual Future<std::pair<char*, size_t> >
    Get(const char* key, size_t key_size,
        std::function<void()> sent = nullptr) override;

    virtual void
    Set(const char* key, size_t key_size,
        const char* val, size_t val_size,
        std::function<void()> sent = nullptr) override;

    virtual Future<std::pair<char*, size_t> >
    SyncGet(const char* key, size_t key_size,
            uint32_t waitfor, std::function<void()> sent = nullptr) override;

    virtual void
    SyncSet(const char* key, size_t key_size,
            const char* val, size_t val_size,
            uint32_t delta, std::function<void()> sent = nullptr) override;

    virtual Future<uint32_t>
    Increment(const char* key,
              size_t key_size,
              std::function<void()> sent = nullptr) override;

    virtual void
    HandleMessage(NetworkId from, const char* buf,
                  size_t len) override;
  private:
    enum DHTOp {
      GET_REQUEST,
      GET_RESPONSE,
      SET_REQUEST,
      SYNC_GET_REQUEST,
      SYNC_SET_REQUEST,
      INCREMENT_REQUEST,
      INCREMENT_RESPONSE
    };

    struct GetRequest {
      DHTOp op;
      unsigned op_id;
    };

    struct GetResponse {
      DHTOp op;
      unsigned op_id;
    };

    struct SetRequest {
      DHTOp op;
      size_t key_size;
    };

    struct SyncGetRequest {
      DHTOp op;
      unsigned op_id;
      uint32_t waitfor;
    };

    struct SyncSetRequest {
      DHTOp op;
      size_t key_size;
      uint32_t delta;
    };

    struct IncrementRequest {
      DHTOp op;
      unsigned op_id;
    };

    struct IncrementResponse {
      DHTOp op;
      unsigned op_id;
      uint32_t val;
    };

    void GetRespond(NetworkId to,
                    unsigned op_id,
                    const std::string& key);

    void HandleGetRequest(NetworkId from, const GetRequest& req,
                          const char* key, size_t len);

    void HandleGetResponse(const GetResponse& resp,
                           const char* val, size_t len);

    void HandleSetRequest(const SetRequest& req,
                          const char* buf, size_t len);

    void HandleSyncGetRequest(NetworkId from, const SyncGetRequest& req,
                              const char* key, size_t len);

    void HandleSyncSetRequest(const SyncSetRequest& req, const char* buf,
                              size_t len);
    void HandleIncrementRequest(NetworkId from, const IncrementRequest& req,
                                const char* key, size_t len);
    void HandleIncrementResponse(const IncrementResponse& resp);

    int rank_;
    size_t node_count_;
    std::atomic_uint op_id_;
    std::unordered_map<unsigned, Promise<uint32_t> > increment_promise_map_;
    std::unordered_map<std::string, uint32_t> value_table_;
    std::unordered_map<std::string, std::string> table_;
    std::unordered_map<std::string,
                       std::pair<uint32_t,
                                 std::multimap<uint32_t,
                                               std::pair<NetworkId,
                                                         unsigned> > > > sync_table_;
    std::unordered_map<unsigned,
                       Promise<std::pair<char*, size_t> > > promise_map_;
  };
}

#endif
