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

#include <iostream>

#include "app/app.hpp"
#include "app/PingPong/Echo.hpp"
#include "ebb/SharedRoot.hpp"
#include "ebb/MessageManager/MessageManager.hpp"

ebbrt::EbbRoot*
ebbrt::Echo::ConstructRoot()
{
  return new SharedRoot<Echo>;
}

// registers symbol for configuration
__attribute__((constructor(65535)))
static void _reg_symbol()
{
  ebbrt::app::AddSymbol ("Echo", ebbrt::Echo::ConstructRoot);
}

ebbrt::Echo::Echo(EbbId id) : EbbRep(id) {}

void
ebbrt::Echo::HandleMessage(NetworkId from,
                           Buffer buffer)
{
  std::cout << buffer.data();

  const char cbuf[] = "PONG\n";
  auto buf = message_manager->Alloc(sizeof(cbuf));
  std::memcpy(buf.data(), cbuf, sizeof(cbuf));
  message_manager->Send(from, ebbid_, std::move(buf));
}
