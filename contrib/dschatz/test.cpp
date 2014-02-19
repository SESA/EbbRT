#include <signal.h>

#include <ebbrt/Context.h>
#include <ebbrt/ContextActivation.h>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/NodeAllocator.h>
#include <ebbrt/Runtime.h>

int main() {
  ebbrt::Runtime runtime;
  ebbrt::Context c(runtime);
  boost::asio::signal_set sig(c.io_service_, SIGINT);
  {
    ebbrt::ContextActivation activation(c);

    // ensure clean quit on ctrl-c
    sig.async_wait([&c](const boost::system::error_code& ec,
                        int signal_number) { c.io_service_.stop(); });

    ebbrt::global_id_map->Set(ebbrt::kFirstStaticUserId, "test\n").Then([](
        ebbrt::Future<void> f) {
      f.Get();
      ebbrt::node_allocator->AllocateNode(
          "/home/dschatz/Work/SESA/EbbRT/baremetal/build/debug/ebbrt.elf32");
    });
  }
  c.Run();

  return 0;
}
