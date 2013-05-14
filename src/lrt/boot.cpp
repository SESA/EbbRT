#include <new>
#include <stdexcept>

#include "app/app.hpp"
#include "lrt/boot.hpp"
#include "lrt/console.hpp"
#include "lrt/event.hpp"
#include "lrt/mem.hpp"
#include "lrt/trans.hpp"

/** @brief Initialization of core lrt functionality.  Only the first core will
 * execute
 */
void
ebbrt::lrt::boot::init()
{
  console::init();
  console::write("Console Initialized\n");

  auto num_cores = event::get_num_cores();

  /* Set up initial system state */
  /* We spin indefinably if any lrt function fails to initialize. */
  if (mem::init(num_cores) &&
      event::init(num_cores) &&
      trans::init(num_cores)) {
    /* spin up remaining cores */
    init_smp(num_cores); 
   /* we should not return from init_smp() */
  }

  while (1)
    ;
}

/** @brief Finish cores initialization and enter application. Parallel
 * execution!
 */
void
ebbrt::lrt::boot::init_cpu()
{
  trans::init_cpu();
  app::start();
}
