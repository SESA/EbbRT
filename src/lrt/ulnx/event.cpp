#include "lrt/mem.hpp"


bool
ebbrt::lrt::event::init(unsigned num_cores)
{
  //return init_arch();
  return true;
}

void
ebbrt::lrt::event::init_cpu()
{
  while(1)
    ;
}

void
ebbrt::lrt::event::_event_altstack_push(uintptr_t val)
{
}

uintptr_t
ebbrt::lrt::event::_event_altstack_pop()
{
  return 0;
}
