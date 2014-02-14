#include <ebbrt/Context.h>
#include <ebbrt/ContextActivation.h>
#include <ebbrt/NodeAllocator.h>
#include <ebbrt/Runtime.h>

#include <cstdlib>
#include <signal.h>


void clean_exit(int)
{
  std::exit(1);
}

int main() {

  // enforce safe cleanup on CTRL+a
  struct sigaction sa;
  memset( &sa, 0, sizeof(sa) );
  sa.sa_handler = clean_exit;
  sigfillset(&sa.sa_mask);
  sigaction(SIGINT,&sa,NULL);

  ebbrt::Runtime runtime;
  ebbrt::Context c(runtime);
  {
    ebbrt::ContextActivation activation(c);
    ebbrt::node_allocator->AllocateNode(
        "/home/jmcadden/work/EbbRT/baremetal/build/debug/ebbrt.elf32");
  }
  c.Run();
}
