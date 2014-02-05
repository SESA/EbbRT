#include <ebbrt/Context.h>
#include <ebbrt/ContextActivation.h>
#include <ebbrt/NodeAllocator.h>
#include <ebbrt/Runtime.h>

int main() {
  ebbrt::Runtime runtime;
  ebbrt::Context c(runtime);
  {
    ebbrt::ContextActivation activation(c);
    ebbrt::node_allocator->AllocateNode(
        "/home/dschatz/Work/SESA/EbbRT/baremetal/build/debug/ebbrt.elf32");
  }
  c.Run();
}
