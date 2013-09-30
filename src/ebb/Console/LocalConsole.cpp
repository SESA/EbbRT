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
#include "ebb/Console/LocalConsole.hpp"
#include "src/lrt/config.hpp"
#include "src/app/app.hpp"

#ifdef __ebbrt__
#include "lrt/bare/assert.hpp"
#endif

#ifdef __linux__
#include <iostream>
#endif

ebbrt::EbbRoot*
ebbrt::LocalConsole::ConstructRoot()
{
  return new SharedRoot<LocalConsole>;
}

// registers symbol for configuration
__attribute__((constructor(65535)))
static void _reg_symbol()
{
  ebbrt::app::AddSymbol ("Console", ebbrt::LocalConsole::ConstructRoot);
}

ebbrt::LocalConsole::LocalConsole(EbbId id) : Console(id) {}

ebbrt::Buffer
ebbrt::LocalConsole::Alloc(size_t size)
{
  auto buf = malloc(size);
  if (buf == nullptr) {
    throw std::bad_alloc();
  }

  return Buffer{buf, size};
}

void
ebbrt::LocalConsole::Write(Buffer buffer)
{
#ifdef __linux__
  std::cout << buffer.data();
#elif __ebbrt__
  lrt::console::write(buffer.data());
#endif
}
