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

  auto num_cores = event::get_num_cores();

  /* Set up initial system state */
  mem::init(num_cores);
  event::init(num_cores);
  trans::init(num_cores);

  /* start secondary cores */
  init_smp(num_cores);
}

#ifdef __ebbrt__

//For c++ static construction/destruction
void* __dso_handle = nullptr;

extern void (*start_ctors[])();
extern void (*end_ctors[])();

#endif

namespace {
  /** Once only construction */
  void construct()
  {
#ifdef __ebbrt__
    for (unsigned i = 0; i < (end_ctors - start_ctors); ++i) {
      start_ctors[i]();
    }
#endif
    ebbrt::lrt::trans::init_ebbs();
  }
}

#ifdef __ebbrt__
/* by default no secondary cores will come up */
bool ebbrt::app::multi __attribute__((weak)) = false;
#endif

void
ebbrt::lrt::boot::init_cpu()
{
  /* per-core translation setup */
  trans::init_cpu();

#ifdef __ebbrt__
  if (app::multi) {
    /* if multi, then construct only on core 0 and spin the others */
    static std::atomic<bool> initialized {false};
    if (event::get_location() == 0) {
      construct();
      initialized = true;
    } else {
      while (initialized == false)
        ;
    }
    app::start();
  } else if (event::get_location() == 0) {
#endif
      construct();
      app::start();
#ifdef __ebbrt__
  }
#endif
}
