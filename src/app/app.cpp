#include <cinttypes>
#include <cstdio>

#include "app/app.hpp"
#include "app/AppEbb.hpp"
#include "ebb/MemoryAllocator/MemoryAllocator.hpp"
#include "lrt/boot.hpp"
#include "lrt/console.hpp"
#include "lrt/event.hpp"
#include "lrt/trans.hpp"
#include "misc/elf.hpp"
#include "sync/compiler.hpp"

#ifdef LRT_ULNX

void
ebbrt::app::start(){
  
  lrt::trans::init_ebbs();

  // Invoke the app ebb
  app_ebb->Start();
}

#elif LRT_BARE

namespace {
  bool flag;
}

//For c++ static construction/destruction
void* __dso_handle = nullptr;

extern void (*start_ctors[])();
extern void (*end_ctors[])();

/**
 * @brief Entry way into the application
 * is app going to be an ebb ? move this to lrt
 */
void
ebbrt::app::start()
{
  if (lrt::event::get_location() == 0) {
    for (unsigned i = 0; i < (end_ctors - start_ctors); ++i) {
      start_ctors[i]();
    }
    lrt::trans::init_ebbs();
    access_once(flag) = true;
  } else {
    while (!access_once(flag))
      ;
  }

  // Invoke the app ebb
  app_ebb->Start();
}

#endif
