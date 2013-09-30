/*
  EbbRT: Distributed, Elastic, Runtime
  Copyright (C) 2013 SESA Group, Boston University

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
4  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <exception>
#include "app/app.hpp"
#include "lrt/bare/boot.hpp"
#include "lrt/bare/console.hpp"
#include "lrt/config.hpp"
#include "lrt/event.hpp"
#include "lrt/bare/mem.hpp"
#include "lrt/trans.hpp"
#include "sync/spinlock.hpp"

void
ebbrt::lrt::boot::init()
{
  console::init();

  auto num_cores = event::get_num_cores();

  /* Set up initial system state */
  mem::init(num_cores);
  event::init(num_cores);

  /* start secondary cores */
  init_smp(num_cores);
}

//For c++ static construction/destruction
void* __dso_handle = nullptr;

extern void (*start_ctors[])();
extern void (*end_ctors[])();

extern char __eh_frame_start[];
extern "C" void __register_frame(void*);

namespace {
  /** Once only construction */
  void construct()
  {
    int early_init_count = ebbrt::lrt::trans::early_init_ebbs();
    __register_frame(__eh_frame_start);
    for (unsigned i = 0; i < (end_ctors - start_ctors); ++i) {
      start_ctors[i]();
    }
    ebbrt::lrt::trans::init_ebbs(early_init_count);
  }

  ebbrt::Spinlock lock;
  void run_app()
  {
    try {
      ebbrt::app::start();
    } catch (std::exception& e) {
      ebbrt::lrt::console::write("Exception caught: ");
      ebbrt::lrt::console::write(e.what());
      ebbrt::lrt::console::write("\n");
    } catch (...) {
      ebbrt::lrt::console::write("Exception caught\n");
    }
  }
}

void
ebbrt::lrt::boot::init_cpu()
{
  /* per-core translation setup */
  trans::init_cpu();

  if (config::get_multicore()) {
    /* if multi, then construct only on core 0 and spin the others */
    static std::atomic<bool> initialized {false};
    if (event::get_location() == 0) {
      construct();
      initialized = true;
    } else {
      while (initialized == false)
        ;
    }
    run_app();
  } else if (event::get_location() == 0) {
    construct();
    run_app();
  }
}
