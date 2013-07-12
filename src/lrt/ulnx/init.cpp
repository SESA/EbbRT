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
#include "ebbrt.hpp"
#include "app/app.hpp"
#include "lrt/event_impl.hpp"
#include "lrt/ulnx/init.hpp"

__thread ebbrt::Context* ebbrt::active_context;

ebbrt::EbbRT::EbbRT() : initialized_{false}, next_id_{0},
  miss_handler_(&lrt::trans::init_root)
{
  initial_root_table_ = new lrt::trans::RootBinding[app::config.num_init];
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

#ifndef __bg__
  //get the epoll fd for the event loop
  epoll_fd_ = epoll_create(1);
  if (epoll_fd_ == -1) {
    throw std::runtime_error("epoll_create failed");
  }
#endif

  location_ = instance_.AllocateLocation();

  //do initialization only on location 0
  if (location_ == 0) {
    // fill in the initial root table
    // note: these create root calls may make ebb calls which is why
    // this is done from within a context and not when we construct
    // the EbbRT
    for (unsigned i = 0; i < app::config.num_init; ++i) {
      instance_.initial_root_table_[i].id =
        lrt::trans::find_static_ebb_id(app::config.init_ebbs[i].name);
      instance_.initial_root_table_[i].root =
        app::config.init_ebbs[i].create_root();
    }
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

#include <iostream>

void
ebbrt::Context::Loop(int count)
{
  int dispatched = 0;
  while (count == -1 || dispatched < count) {
#ifndef __bg__
    struct epoll_event epoll_event;
    //blocks until an event is ready
    if (epoll_wait(epoll_fd_, &epoll_event, 1, -1) == -1) {
      if (errno == EINTR) {
        continue;
      }
      throw std::runtime_error("epoll_wait failed");
    }
    ebbrt::lrt::event::_event_interrupt(epoll_event.data.u32);
#else
    while (1) {
      // Workaround for CNK bug
      if (fds_.size() != 0) {
        int ret = poll(fds_.data(), fds_.size(), 0);
        if (ret == -1) {
          if (errno == EINTR) {
            continue;
          }
          throw std::runtime_error("poll failed");
        }
        for (unsigned i = 0; i < fds_.size(); ++i) {
          if (fds_[i].revents) {
            ebbrt::lrt::event::_event_interrupt(interrupts_[i]);
            break;
          }
        }
      }
      //call functions
      std::vector<std::function<int()> > funcs_copy{funcs_};
      bool didevent = false;
      for (const auto& func: funcs_copy) {
        int interrupt = func();
        if (interrupt >= 0) {
          didevent = true;
          ebbrt::lrt::event::_event_interrupt(interrupt);
          break;
        }
      }
      if (didevent) {
        break;
      }
    }
#endif
    ++dispatched;
  }
}
