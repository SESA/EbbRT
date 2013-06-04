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
#include "ebb/Console/Console.hpp"
#include "ebb/MessageManager/MessageManager.hpp"

#ifdef __ebbrt__
#include "lrt/bare/assert.hpp"
#endif

#ifdef __linux__
#include <iostream>
#endif
ebbrt::EbbRoot*
ebbrt::Console::ConstructRoot()
{
  return new SharedRoot<Console>;
}

void
ebbrt::Console::Write(const char* str,
                      std::function<void()> cb)
{
#ifdef __linux__
  std::cout << str;
  if (cb) {
    cb();
  }
#elif __ebbrt__
  BufferList list = BufferList(1, std::make_pair(str, strlen(str) + 1));
  LRT_ASSERT(!cb);
  NetworkId id;
  id.mac_addr[0] = 0xff;
  id.mac_addr[1] = 0xff;
  id.mac_addr[2] = 0xff;
  id.mac_addr[3] = 0xff;
  id.mac_addr[4] = 0xff;
  id.mac_addr[5] = 0xff;
  message_manager->Send(id, console, std::move(list));
#endif
}

void
ebbrt::Console::HandleMessage(const uint8_t* msg,
                              size_t len)
{
#ifdef __linux__
  std::cout << msg;
#elif __ebbrt__
  LRT_ASSERT(0);
#endif
}
