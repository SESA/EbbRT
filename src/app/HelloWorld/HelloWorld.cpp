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

#include <array>
#include <mutex>

#include "app/app.hpp"
#include "ebb/EbbManager/PrimitiveEbbManager.hpp"
#include "ebb/MemoryAllocator/SimpleMemoryAllocator.hpp"

#ifdef __linux__
#include <iostream>
#include <thread>
#include "ebbrt.hpp"
#elif __ebbrt
#include "ebb/Gthread/Gthread.hpp"
#include "ebb/Syscall/Syscall.hpp"
#include "lrt/bare/console.hpp"
#include "sync/spinlock.hpp"
#endif

constexpr ebbrt::app::Config::InitEbb init_ebbs[] =
{
  {
    .create_root = ebbrt::SimpleMemoryAllocatorConstructRoot,
    .name = "MemoryAllocator"
  },
  {
    .create_root = ebbrt::PrimitiveEbbManagerConstructRoot,
    .name = "EbbManager"
  },
#ifdef __ebbrt__
  {
    .create_root = ebbrt::Gthread::ConstructRoot,
    .name = "Gthread"
  },
  {
    .create_root = ebbrt::Syscall::ConstructRoot,
    .name = "Syscall"
  }
#endif
};

constexpr ebbrt::app::Config::StaticEbbId static_ebbs[] = {
  {.name = "MemoryAllocator", .id = 1},
  {.name = "EbbManager", .id = 2},
  {.name = "Gthread", .id = 3},
  {.name = "Syscall", .id = 4}
};

const ebbrt::app::Config ebbrt::app::config = {
  .space_id = 0,
#ifdef __ebbrt__
  .num_early_init = 4,
#endif
  .num_init = sizeof(init_ebbs) / sizeof(Config::InitEbb),
  .init_ebbs = init_ebbs,
  .num_statics = sizeof(static_ebbs) / sizeof(Config::StaticEbbId),
  .static_ebb_ids = static_ebbs
};

#ifdef __ebbrt__
bool ebbrt::app::multi = true;
#endif

void
ebbrt::app::start()
{
#ifdef __linux__
  static std::mutex mutex;
  std::lock_guard<std::mutex> lock(mutex);
  std::cout << "Hello World" << std::endl;
#elif __ebbrt__
  static Spinlock lock;
  lock.Lock();
  lrt::console::write("Hello World\n");
  lock.Unlock();
#endif
}

#ifdef __linux__
int main()
{
  ebbrt::EbbRT instance;

  std::thread threads[std::thread::hardware_concurrency()];
  for (auto& thread : threads) {
    thread = std::thread([&]{ebbrt::Context context{instance};});
  }
  for (auto& thread : threads) {
    thread.join();
  }
  return 0;
}
#endif
