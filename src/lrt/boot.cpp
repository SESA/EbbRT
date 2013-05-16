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

  /* Set up initial system state */
  mem::init(num_cores);
  event::init(num_cores);
  trans::init(num_cores);

  /* wake up secondary cores */
  init_smp(num_cores); 
}

void
ebbrt::lrt::boot::init_cpu()
{
  /* per-core translation setup */
  trans::init_cpu();

  /* Enter the wild world of ebbs */
  app::start();
}
