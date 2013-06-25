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
#include "ebb/SharedRoot.hpp"
#include "ebb/FileSystem/FileSystem.hpp"
#include "ebb/MessageManager/MessageManager.hpp"

#ifdef __linux__
#include "ebb/FileSystem/FileStream.hpp"
#endif

ebbrt::EbbRoot*
ebbrt::FileSystem::ConstructRoot()
{
  return new SharedRoot<FileSystem>;
}

ebbrt::FileSystem::FileSystem() : op_id_{0}
{
}

void
ebbrt::FileSystem::OpenReadStream(const char* filename,
                                  OpenCallback callback,
                                  uint64_t offset,
                                  int64_t length)
{
#ifdef __linux__
  assert(0);
#elif __ebbrt__
  // Create message
  auto message = new OpenReadMessage;
  // get a new op_id
  auto op_id = op_id_.fetch_add(1, std::memory_order_relaxed);
  message->op_id = op_id;
  message->op = FSOp::OPEN_READ;
  message->offset = offset;
  message->length = length;

  // Construct the buffer list
  BufferList list {
    std::pair<const void*, size_t>(message, sizeof(OpenReadMessage)),
      std::pair<const void*, size_t>(filename, strlen(filename) + 1)};

  //FIXME: currently hardcoded network id
  NetworkId id;
  id.mac_addr[0] = 0xff;
  id.mac_addr[1] = 0xff;
  id.mac_addr[2] = 0xff;
  id.mac_addr[3] = 0xff;
  id.mac_addr[4] = 0xff;
  id.mac_addr[5] = 0xff;

  // store callback for later
  lock_.Lock();
  cb_map_[op_id] = callback;
  lock_.Unlock();

  // Send message
  message_manager->Send(id, file_system, std::move(list),
                        //Make sure to free the message when done
                        [=]() {
                          free(message);
                        });
#endif
}

void
ebbrt::FileSystem::HandleOpenRead(NetworkId from,
                                  const OpenReadMessage& message)
{
#ifdef __linux__
  // Construct new file stream
  // EbbRef<FileStream> file_stream{ebb_manager->AllocateId()};
  // ebb_manager->Bind(FileStream::ConstructRoot, file_stream);
  //FIXME: We can't do remote misses yet so this is statically built

  //FIXME: Because we can't pass parameters to constructors yet
  file_stream->Open(message.filename, message.offset, message.length);

  // Construct Reply
  auto new_message = new OpenReadReplyMessage;
  new_message->op = OPEN_READ_REPLY;
  new_message->op_id = message.op_id;
  new_message->stream_id = file_stream;

  BufferList list {
    std::pair<const void*, size_t>(new_message, sizeof(*new_message))
  };

  // Send Reply
  message_manager->Send(from, file_system, std::move(list),
                        [=]() {
                          free(new_message);
                        });
#elif __ebbrt__
  LRT_ASSERT(0);
#endif
}

void
ebbrt::FileSystem::HandleOpenReadReply(NetworkId from,
                                       const OpenReadReplyMessage& message)
{
#if __linux__
  assert(0);
#elif __ebbrt__
  // find callback
  lock_.Lock();
  auto it = cb_map_.find(message.op_id);
  LRT_ASSERT(it != cb_map_.end());
  auto& cb = it->second;
  lock_.Unlock();

  // call it!
  cb(EbbRef<Stream>(message.stream_id));
#endif
}

void
ebbrt::FileSystem::HandleMessage(NetworkId from,
                                 const char* buf,
                                 size_t len)
{
#ifdef __linux__
  assert(len >= sizeof(FSOp));
#elif __ebbrt__
  LRT_ASSERT(len >= sizeof(FSOp));
#endif
  auto op = reinterpret_cast<const FSOp*>(buf);
  switch (*op) {
  case FSOp::OPEN_READ:
#ifdef __linux__
    assert(len >= sizeof(OpenReadMessage));
#elif __ebbrt__
    LRT_ASSERT(len >= sizeof(OpenReadMessage));
#endif
    HandleOpenRead(from, *reinterpret_cast<const OpenReadMessage*>(buf));
    break;
  case FSOp::OPEN_READ_REPLY:
#ifdef __linux__
    assert(len >= sizeof(OpenReadReplyMessage));
#elif __ebbrt__
    LRT_ASSERT(len >= sizeof(OpenReadReplyMessage));
#endif
    HandleOpenReadReply(from,
                        *reinterpret_cast<const OpenReadReplyMessage*>(buf));
    break;
  }
}
