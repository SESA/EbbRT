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
#ifndef EBBRT_EBB_FILESYSTEM_FILESYSTEM_HPP
#define EBBRT_EBB_FILESYSTEM_FILESYSTEM_HPP

#include <functional>
#include <string>

#include "ebb/ebb.hpp"
#include "ebb/Stream/Stream.hpp"

namespace ebbrt {
  class FileSystem : public EbbRep {
  public:
    static EbbRoot* ConstructRoot();

    FileSystem();

    typedef std::function<void(EbbRef<Stream>)> OpenCallback;
    virtual void OpenReadStream(const char* filename,
                                OpenCallback callback,
                                uint64_t offset = 0,
                                int64_t length = -1);


    virtual void HandleMessage(NetworkId from,
                               const char* buf,
                               size_t len) override;
  private:
    enum FSOp {
      OPEN_READ,
      OPEN_READ_REPLY
    };

    class OpenReadMessage {
    public:
      FSOp op;
      unsigned op_id;
      uint64_t offset;
      int64_t length;
      char filename[0];
    };

    class OpenReadReplyMessage {
    public:
      FSOp op;
      unsigned op_id;
      EbbId stream_id;
    };

    void HandleOpenRead(NetworkId id,
                        const OpenReadMessage& message);
    void HandleOpenReadReply(NetworkId id,
                             const OpenReadReplyMessage& message);

    std::atomic_uint op_id_;
    Spinlock lock_;
    std::unordered_map<unsigned, OpenCallback> cb_map_;
  };
  const EbbRef<FileSystem> file_system =
    EbbRef<FileSystem>(lrt::trans::find_static_ebb_id("FileSystem"));
}
#endif
