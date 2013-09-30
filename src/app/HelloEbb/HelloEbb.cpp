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

#include <mutex>
#include <vector>

#include "app/app.hpp"
#include "ebb/Console/LocalConsole.hpp"
#include "ebb/EbbManager/PrimitiveEbbManager.hpp"
#include "ebb/EventManager/SimpleEventManager.hpp"
#include "ebb/MemoryAllocator/SimpleMemoryAllocator.hpp"

#ifdef __linux__
#include <iostream>
#include <thread>
#include "ebbrt.hpp"
#elif __ebbrt__
#include "ebb/Gthread/Gthread.hpp"
#include "ebb/Syscall/Syscall.hpp"
#include "lrt/bare/console.hpp"
#include "sync/spinlock.hpp"
#endif


#ifdef __ebbrt__
bool ebbrt::app::multi = true;

void
ebbrt::app::start()
{
  static Spinlock lock;
  lock.Lock();
  char str[] = "Hello Ebb\n";
  auto buf = ebbrt::console->Alloc(sizeof(str));
  std::memcpy(buf.data(), str, sizeof(str));
  ebbrt::console->Write(std::move(buf));
  lock.Unlock();
}
#endif

#ifdef __linux__
int main()
{
  ebbrt::EbbRT instance;

  std::vector<std::thread> threads(std::thread::hardware_concurrency());
  for (auto& thread : threads) {
    thread = std::thread([&]{
      ebbrt::Context context{instance};
      context.Activate();
      char str[] = "Hello Ebb\n";
      auto buf = ebbrt::console->Alloc(sizeof(str));
      std::memcpy(buf.data(), str, sizeof(str));
      ebbrt::console->Write(std::move(buf));
      context.Deactivate();
      });
  }
  for (auto& thread : threads) {
    thread.join();
  }
  return 0;
}
#endif
