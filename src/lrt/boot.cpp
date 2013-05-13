#include <new>
#include <stdexcept>

#include "app/app.hpp"
#include "lrt/boot.hpp"
#include "lrt/console.hpp"
#include "lrt/event.hpp"
#include "lrt/mem.hpp"
#include "lrt/trans.hpp"

void
ebbrt::lrt::boot::init()
{
  console::init();
  console::write("Console Initialized\n");

  auto num_cores = event::get_num_cores();

  if (mem::init(num_cores) &&
      event::init(num_cores) &&
      trans::init(num_cores)) {
    init_smp(num_cores);
  }

  while (1)
    ;
}

void
ebbrt::lrt::boot::init_cpu()
{
  trans::init_cpu();
  app::start();
}
