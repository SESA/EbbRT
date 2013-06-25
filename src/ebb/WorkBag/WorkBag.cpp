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
#include <cstring>

#include "ebb/SharedRoot.hpp"
#include "ebb/WorkBag/WorkBag.hpp"
#include "ebb/MessageManager/MessageManager.hpp"

ebbrt::EbbRoot*
ebbrt::WorkBag::ConstructRoot()
{
  return new SharedRoot<WorkBag>;
}

void
ebbrt::WorkBag::Get(callback cb)
{
#ifdef __linux__
  lock_.lock();
  if (!bag_.empty()) {
    auto f = bag_.front();
    bag_.pop_front();
    lock_.unlock();
    cb(f.first, f.second);
  } else {
    lock_.unlock();
    cb(nullptr, 0);
  }
#elif __ebbrt__
  NetworkId id;
  id.mac_addr[0] = 0xff;
  id.mac_addr[1] = 0xff;
  id.mac_addr[2] = 0xff;
  id.mac_addr[3] = 0xff;
  id.mac_addr[4] = 0xff;
  id.mac_addr[5] = 0xff;
  lock_.Lock();
  cbs_.push_front(std::move(cb));
  lock_.Unlock();
  BufferList list;
  message_manager->Send(id, workbag, std::move(list));
#endif
}

void
ebbrt::WorkBag::Add(char* data, size_t size)
{
#ifdef __linux__
  char* newdata = new char[size];
  memcpy(newdata, data, size);
  lock_.lock();
  bag_.emplace_front(newdata, size);
  lock_.unlock();
#elif __ebbrt__
  LRT_ASSERT(0);
#endif
}

void
ebbrt::WorkBag::HandleMessage(NetworkId from,
                              const char* buf,
                              size_t len)
{
#ifdef __linux__
  Get([=](const char* data, size_t size) {
      BufferList list;
      if (data != nullptr) {
        list.emplace_front(data, size);
        message_manager->Send(from, workbag, std::move(list),
                              [=] () {
                                free(const_cast<char*>(data));
                              });
      } else {
        message_manager->Send(from, workbag, std::move(list));
      }
    });
#elif __ebbrt__
  lock_.Lock();
  LRT_ASSERT(!cbs_.empty());
  auto cb = cbs_.front();
  cbs_.pop_front();
  lock_.Unlock();
  if (len > 0) {
    cb(buf, len);
  } else {
    cb(nullptr, len);
  }
#endif
}
