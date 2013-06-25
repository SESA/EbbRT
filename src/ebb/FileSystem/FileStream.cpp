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

#include <algorithm>

#include "ebb/DistributedRoot.hpp"
#include "ebb/EventManager/EventManager.hpp"
#include "ebb/FileSystem/FileStream.hpp"
#include "ebb/MessageManager/MessageManager.hpp"

#ifdef __linux__
#include <fcntl.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#endif

ebbrt::EbbRoot*
ebbrt::FileStream::ConstructRoot()
{
  return new DistributedRoot<FileStream>;
}

ebbrt::FileStream::FileStream()
{
#ifdef __linux__
  //FIXME: make configurable
  credits_ = 5;
  event_fd_ = eventfd(0, EFD_NONBLOCK);
  if (event_fd_ == -1) {
    throw std::runtime_error("eventfd failed");
  }
  uint8_t interrupt = event_manager->AllocateInterrupt([]() {
      file_stream->Send();
    });
  event_manager->RegisterFD(event_fd_, EPOLLIN, interrupt);
#endif
}

void
ebbrt::FileStream::Open(const char* filename, uint64_t offset, int64_t length)
{
#ifdef __linux__
  fd_ = open(filename, O_RDONLY);
  if (fd_ == -1) {
    throw std::runtime_error("Open failed");
  }

  offset_ = offset;
  length_ = length;
#elif __ebbrt__
  LRT_ASSERT(0);
#endif
}

void
ebbrt::FileStream::Attach(std::function<void(const char*, size_t)> processor,
                          Split how_to_split)
{
#ifdef __linux__
  assert(0);
#elif __ebbrt__
  // store callback for later
  lock_.Lock();
  LRT_ASSERT(!processor_);
  processor_ = std::move(processor);
  lock_.Unlock();

  // construct message
  auto message = new AttachMessage;
  message->op = ATTACH;
  message->how_to_split = how_to_split;
  BufferList list{std::pair<const void*, size_t>(message,
                                                 sizeof(AttachMessage))};

  //FIXME: currently hardcoded network id
  NetworkId id;
  id.mac_addr[0] = 0xff;
  id.mac_addr[1] = 0xff;
  id.mac_addr[2] = 0xff;
  id.mac_addr[3] = 0xff;
  id.mac_addr[4] = 0xff;
  id.mac_addr[5] = 0xff;


  // Send message
  message_manager->Send(id, file_stream, std::move(list),
                        //Make sure to free the message when done
                        [=]() {
                          free(message);
                        });
#endif
}

void
ebbrt::FileStream::Attach(EbbRef<StreamProcessor> processor,
                          Split how_to_split)
{
#ifdef __linux__
  assert(0);
#elif __ebbrt__
  LRT_ASSERT(0);
#endif
}

void
ebbrt::FileStream::EnableSend()
{
#ifdef __linux__
  uint64_t val = 1;
  int ret = write(event_fd_, reinterpret_cast<void*>(&val), 8);
  if (ret == -1) {
    throw std::runtime_error("Write failed");
  }
#elif __ebbrt__
  LRT_ASSERT(0);
#endif
}

void
ebbrt::FileStream::HandleAttach(NetworkId from,
                                const AttachMessage& message)
{
#ifdef __linux__
  remote_ = from;

  EnableSend();
#elif __ebbrt__
  LRT_ASSERT(0);
#endif
}

void
ebbrt::FileStream::HandleData(NetworkId from,
                              const DataMessage& message,
                              size_t len)
{
#ifdef __linux__
  assert(0);
#elif __ebbrt__
  LRT_ASSERT(processor_);
  processor_(message.data, len);

  auto newmessage = new CreditMessage;
  newmessage->op = CREDIT;
  BufferList list{std::pair<const void*, size_t>(newmessage,
                                                 sizeof(CreditMessage))};

  message_manager->Send(from, file_stream, std::move(list),
                        [=]() {
                          free(newmessage);
                        });
#endif
}

void
ebbrt::FileStream::HandleCredit()
{
#ifdef __linux__
  //FIXME: race condition here
  if (credits_ == 0) {
    EnableSend();
  }
  credits_++;
#elif __ebbrt__
  LRT_ASSERT(0);
#endif
}


void
ebbrt::FileStream::HandleMessage(NetworkId from,
                                 const char* buf,
                                 size_t len)
{
#ifdef __linux__
  assert(len >= sizeof(Op));
#elif __ebbrt__
  LRT_ASSERT(len >= sizeof(Op));
#endif
  auto op = reinterpret_cast<const Op*>(buf);
  switch (*op) {
  case Op::ATTACH:
#ifdef __linux__
    assert(len >= sizeof(AttachMessage));
#elif __ebbrt__
    LRT_ASSERT(len >= sizeof(AttachMessage));
#endif
    HandleAttach(from, *reinterpret_cast<const AttachMessage*>(buf));
    break;
  case Op::DATA:
#ifdef __linux__
    assert(len >= sizeof(DataMessage));
#elif __ebbrt__
    LRT_ASSERT(len >= sizeof(DataMessage));
#endif
    HandleData(from, *reinterpret_cast<const DataMessage*>(buf),
               len - sizeof(DataMessage));
    break;
  case Op::CREDIT:
    HandleCredit();
    break;
  }
}

#ifdef __linux__
void
ebbrt::FileStream::Send()
{
  //FIXME: there is a race here
  assert(credits_ != 0);
  credits_--;
  if (credits_ == 0) {
    //disable the event
    char tmp[8];
    auto ret = read(event_fd_, tmp, 8);
    if (ret == -1) {
      throw std::runtime_error("read failed");
    }
  }

  //FIXME: figure out max size or make it configurable
  auto askfor = length_ == -1 ? 1024 : std::max(length_,
      static_cast<int64_t>(1024));
  auto buffer = new char[askfor];
  auto ret = pread(fd_, buffer, askfor, offset_);
  if (ret == -1) {
    throw std::runtime_error("pread failed");
  }

  offset_ += ret;
  if (length_ != -1) {
    length_ -= ret;
  }

  auto message = new DataMessage;
  message->op = DATA;
  BufferList list{
    std::pair<const void*, size_t>(message, sizeof(DataMessage)),
      std::pair<const void*, size_t>(buffer, ret)};

  message_manager->Send(remote_, file_stream, std::move(list),
                        [=]() {
                          free(message);
                          delete[] buffer;
                        });

  if (ret == 0 || length_ == 0) {
    std::cout << "Stop Sending!" << std::endl;
  }
}
#endif
