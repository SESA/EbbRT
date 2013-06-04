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

#ifdef __linux__
#include <iostream>
#include <thread>
#include "ebbrt.hpp"
#elif __ebbrt
#include "lrt/bare/console.hpp"
#include "sync/spinlock.hpp"
#endif

const ebbrt::app::Config ebbrt::app::config = {
  .space_id = 0,
  .num_init = 0,
  .init_ebbs = nullptr,
  .num_statics = 0,
  .static_ebb_ids = nullptr
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
