#include <new>

#include "lrt/event_impl.hpp"
#include "lrt/mem.hpp"

uintptr_t** ebbrt::lrt::event::altstack;

void
ebbrt::lrt::event::init(unsigned num_cores)
{
  /* allocate the array of pointers to alternative stacks */
  altstack = new (mem::malloc(sizeof(uintptr_t*) * num_cores, 0))
    uintptr_t*[num_cores];


  init_arch();
}

void
ebbrt::lrt::event::_event_altstack_push(uintptr_t val)
{
  *(altstack[get_location()]++) = val;
}

uintptr_t
ebbrt::lrt::event::_event_altstack_pop()
{
  return *(--altstack[get_location()]);
}
