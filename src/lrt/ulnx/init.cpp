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
#include <cassert>

#include "ebbrt.hpp"
#include "app/app.hpp"
#include "lrt/config.hpp"
#include "lrt/event_impl.hpp"
#include "lib/fdt/libfdt.h"
#include "lrt/ulnx/init.hpp"

__thread ebbrt::Context* ebbrt::active_context;

ebbrt::EbbRT::EbbRT() : initialized_{false}, next_id_{0},
  miss_handler_(&lrt::trans::init_root)
{
  //initial_root_table_ = new lrt::trans::RootBinding[app::config.num_late_init];
  initial_root_table_ = new lrt::trans::RootBinding[lrt::config::get_static_ebb_count()];
}

ebbrt::lrt::event::Location
ebbrt::EbbRT::AllocateLocation()
{
  //TODO: make this allow for freeing of locations
  assert(next_id_ < lrt::event::get_max_contexts());
  return next_id_++;
}

ebbrt::Context::Context(EbbRT& instance) : instance_(instance)
{
  active_context = nullptr;
  char* fdt = nullptr;

  location_ = instance_.AllocateLocation();

  //do initialization only on location 0
  if (location_ == 0) {
    // fill in the initial root table
    // note: these create root calls may make ebb calls which is why
    // this is done from within a context and not when we construct
    // the EbbRT
#if 0
    for (unsigned i = 0; i < lrt::config::get_static_ebb_count(); ++i) {

      instance_.initial_root_table_[i].id =
        lrt::trans::find_static_ebb_id(app::config.late_init_ebbs[i].name);

      ebbrt::app::ConfigFuncPtr func =
        ebbrt::app::LookupSymbol(app::config.late_init_ebbs[i].name);
      assert( func != nullptr );// lookup failed
      instance_.initial_root_table_[i].root = func();
    }
#endif

  int ebbs =  fdt_path_offset(fdt, "/ebbs");
  int nextebb = fdt_first_subnode(fdt, ebbs);
  while( nextebb > 0) 
  {
    const char *name;
    int len;
    int i=0;

    name = fdt_get_name(fdt, nextebb, &len);
    uint32_t id = ebbrt::lrt::config::fdt_getint32(nextebb, "id");
    uint32_t early = ebbrt::lrt::config::fdt_getint32(nextebb, "early_init_ebbrt");

    if(early){
      instance_.initial_root_table_[i].id = id;
      ebbrt::app::ConfigFuncPtr func = ebbrt::app::LookupSymbol(name);
      assert( func != nullptr );// lookup failed
      instance_.initial_root_table_[i].root = func();
      i++;
    }
    nextebb = fdt_next_subnode(fdt, nextebb);
  }

    /////////
    std::lock_guard<std::mutex> lock(instance_.init_lock_);
    instance_.initialized_ = true;
    instance_.init_cv_.notify_all();
  } else {
    std::unique_lock<std::mutex> lock{instance_.init_lock_};
    if (!instance_.initialized_) {
      instance_.init_cv_.wait(lock,
                              [&]{return instance_.initialized_;});
    }
  }
}

void
ebbrt::Context::Activate(){
  active_context = this;
};
void
ebbrt::Context::Deactivate(){
  active_context = nullptr;
};

void
ebbrt::Context::Loop(int count)
{
  break_loop_ = false;
  int dispatched = 0;
  while (!break_loop_ && (count == -1 || dispatched < count)) {
    lrt::event::process_event();
    ++dispatched;
  }
}

void
ebbrt::Context::Break()
{
  break_loop_ = true;
}
