#include <new>

#include "lrt/event_impl.hpp"
#include "lrt/mem.hpp"

uintptr_t** ebbrt::lrt::event::altstack;

/**
 * @brief Initialize data structures for event handling 
 *
 * @param num_cores amount of cores spun-up on boot
 *
 * @return success/fail
 */
bool
ebbrt::lrt::event::init(unsigned num_cores)
{
  altstack = new (mem::malloc(sizeof(uintptr_t*) * num_cores, 0))
    uintptr_t*[num_cores];
  return init_arch();
}

/**
 * @brief Each core configured in preparation to receive events
 */
void
ebbrt::lrt::event::init_cpu()
{
  init_cpu_arch();
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
