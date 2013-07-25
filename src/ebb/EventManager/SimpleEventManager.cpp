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
#include "ebb/DistributedRoot.hpp"
#include "ebb/EventManager/SimpleEventManager.hpp"
#ifdef __ebbrt__
#include "lrt/bare/assert.hpp"
#endif
#include "app/app.hpp"

// registers symbol for configuration
__attribute__((constructor(65535)))
static void _reg_symbol()
{
  ebbrt::app::AddSymbol ("EventManager", 
			 ebbrt::SimpleEventManager::ConstructRoot);
}

ebbrt::EbbRoot*
ebbrt::SimpleEventManager::ConstructRoot()
{
  return new DistributedRoot<SimpleEventManager>();
}

ebbrt::SimpleEventManager::SimpleEventManager() : next_{32}
{
#ifdef __linux
#ifndef __bg__
  //get the epoll fd for the event loop
  epoll_fd_ = epoll_create(1);
  if (epoll_fd_ == -1) {
    throw std::runtime_error("epoll_create failed");
  }
#endif
#endif
}

uint8_t
ebbrt::SimpleEventManager::AllocateInterrupt(std::function<void()> func)
{
  uint8_t ret = next_++;
  map_.insert(std::make_pair(ret, std::move(func)));
  return ret;
}

void
ebbrt::SimpleEventManager::Async(std::function<void()> func)
{
  asyncs_.push(std::move(func));
}

#ifdef __linux__
void
ebbrt::SimpleEventManager::RegisterFD(int fd,
                                      uint32_t events,
                                      uint8_t interrupt)
{
#ifndef __bg__
  struct epoll_event event;
  event.events = events;
  event.data.u32 = interrupt;
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) == -1) {
    throw std::runtime_error("epoll_ctl failed");
  }
#else
  assert((events | EPOLLIN) || (events | EPOLLOUT));
  short poll_events = 0;
  if (events | EPOLLIN) {
    poll_events |= POLLIN;
  }
  if (events | EPOLLOUT) {
    poll_events |= POLLOUT;
  }

  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = poll_events;

  fds_.push_back(pfd);
  interrupts_.push_back(interrupt);
#endif
}
#endif

#ifdef __bg__
void
ebbrt::SimpleEventManager::RegisterFunction(std::function<int()> func)
{
  funcs_.push_back(std::move(func));
}
#endif

void
ebbrt::SimpleEventManager::HandleInterrupt(uint8_t interrupt)
{
#ifdef __linux__
  assert(map_.find(interrupt) != map_.end());
#elif __ebbrt__
  LRT_ASSERT(map_.find(interrupt) != map_.end());
#endif
  const auto& f = map_[interrupt];
  if (f) {
    f();
  }
}

void
ebbrt::SimpleEventManager::ProcessEvent()
{
  //Check if we have a function to asynchronously call
  if (!asyncs_.empty()) {
    auto f = std::move(asyncs_.top());
    asyncs_.pop();
    f();
    return;
  }

#ifdef __linux__
#ifndef __bg__
  struct epoll_event epoll_event;
  //blocks until an event is ready
  while (epoll_wait(epoll_fd_, &epoll_event, 1, -1) == -1) {
    if (errno != EINTR) {
      throw std::runtime_error("epoll_wait failed");
    }
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
#else
  asm volatile ("sti;"
                "hlt;"
                "cli;"
                :
                :
                : "rax", "rcx", "rdx", "rsi",
                  "rdi", "r8", "r9", "r10", "r11");
#endif
}
