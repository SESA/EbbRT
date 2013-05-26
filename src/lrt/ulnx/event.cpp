#include "lrt/event.hpp"
#include "lrt/mem.hpp"

namespace{
const int STACK_SIZE = 1<<14; //16k
}


bool
ebbrt::lrt::event::init_arch(){

  altstack[get_location()] = reinterpret_cast<uintptr_t*>
    (mem::malloc(STACK_SIZE, get_location()));

  return 1;

}
void
ebbrt::lrt::event::init_cpu_arch(){while(1);}
