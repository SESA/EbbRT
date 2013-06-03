#include <sys/epoll.h>

#include <cerrno>
#include <cstdio>

#include "ebb/EventManager/EventManager.hpp"
#include "lrt/boot.hpp"
#include "lrt/event.hpp"
#include "lrt/mem.hpp"

namespace {
  constexpr int STACK_SIZE = 1<<14; //16k
  int epoll_fd;
}


bool
ebbrt::lrt::event::init_arch(){

  altstack[get_location()] = reinterpret_cast<uintptr_t*>
    (mem::malloc(STACK_SIZE, get_location()));

  return 1;

}

void
ebbrt::lrt::event::init_cpu()
{
  epoll_fd = epoll_create(1);
  if (epoll_fd == -1) {
    perror("epoll_create");
    exit(-1);
  }
  lrt::boot::init_cpu();
  struct epoll_event epoll_event;
  while (1) {
    if (epoll_wait(epoll_fd, &epoll_event, 1, -1) == -1) {
      if (errno == EINTR) {
        continue;
      }
      perror("epoll_wait");
      exit(-1);
    }
    _event_interrupt(epoll_event.data.u32);
  }
}

void
ebbrt::lrt::event::_event_interrupt(uint8_t interrupt)
{
  event_manager->HandleInterrupt(interrupt);
}

void
ebbrt::lrt::event::register_fd(int fd, uint32_t events, uint8_t interrupt)
{
  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.u32 = interrupt;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
    perror("epoll_ctl");
    exit(-1);
  }
}
