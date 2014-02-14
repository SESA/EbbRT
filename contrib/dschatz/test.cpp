#include <cstdlib>
#include <signal.h>

#include <ebbrt/Context.h>
#include <ebbrt/ContextActivation.h>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/NodeAllocator.h>
#include <ebbrt/Runtime.h>

std::atomic<bool> quit;

void clean_exit(int)
{
  quit = true;
}

int main() {
  quit = false;
  // enforce safe cleanup on CTRL+c
  struct sigaction sa;
  memset( &sa, 0, sizeof(sa) );
  sa.sa_handler = clean_exit;
  sigfillset(&sa.sa_mask);
  sigaction(SIGINT,&sa,NULL);

  ebbrt::Runtime runtime;
  ebbrt::Context c(runtime);
  {
    ebbrt::ContextActivation activation(c);
    ebbrt::global_id_map->Set(ebbrt::kFirstStaticUserId, "test\n").Then([](
        ebbrt::Future<void> f) {
      f.Get();
      ebbrt::node_allocator->AllocateNode(
          "/home/dschatz/Work/SESA/EbbRT/baremetal/build/debug/ebbrt.elf32");
    });
  }
  while (!quit) {
    c.PollOne();
  }

  return 0;
}
