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
#ifndef EBBRT_EBB_FILESTREAM_FILESTREAM_HPP
#define EBBRT_EBB_FILESTREAM_FILESTREAM_HPP

#include <vector>

#include "ebb/ebb.hpp"
#include "ebb/Stream/Stream.hpp"

namespace ebbrt {
  class FileStream : public Stream {
  public:
    static EbbRoot* ConstructRoot();

    FileStream();

    virtual void Open(const char* filename, uint64_t offset, int64_t length);

    typedef std::function<void(const char*, size_t)> Processor;
    virtual void Attach(Processor processor,
                        Split how_to_split) override;
    virtual void Attach(EbbRef<StreamProcessor> processor,
                        Split how_to_split) override;

    virtual void HandleMessage(NetworkId from,
                               const char* buf,
                               size_t len) override;
  private:
    enum Op {
      ATTACH,
      DATA,
      CREDIT
    };

    class AttachMessage {
    public:
      Op op;
      Split how_to_split;
    };

    class DataMessage {
    public:
      Op op;
      char data[0];
    };

    class CreditMessage {
    public:
      Op op;
    };

    void EnableSend();
    void HandleAttach(NetworkId from,
                      const AttachMessage& message);
    void HandleData(NetworkId from,
                    const DataMessage& message,
                    size_t len);
    void HandleCredit();
    void HandleSend();

#ifdef __linux__
    int fd_;
    uint64_t offset_;
    int64_t length_;
    NetworkId remote_;
    int credits_;
    int event_fd_;
    void Send();
#elif __ebbrt__
    Spinlock lock_;
    Processor processor_;
#endif
  };
  //FIXME: We can't do remote misses yet so this is statically built
  const EbbRef<FileStream> file_stream =
    EbbRef<FileStream>(lrt::trans::find_static_ebb_id("FileStream"));
}

#endif
