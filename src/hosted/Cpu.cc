#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include <signal.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <array>

#include <boost/container/static_vector.hpp>
#include <boost/filesystem.hpp>

#include "../Clock.h"
#include "../GlobalIdMap.h"
#include "../Runtime.h"
#include "../StaticIds.h"
#include "Context.h"
#include "ContextActivation.h"
#include "NodeAllocator.h"

#include "Cpu.h"

//#define __TRACE_SPAWN__

extern void AppMain();

void Main(void);

namespace ebbrt {
// static memebers
std::array<ExplicitlyConstructed<ebbrt::Cpu>, ebbrt::Cpu::kMaxCpus> cpus;
size_t Cpu::numCpus_ = 0;
std::atomic<int> Cpu::shutdown_;
Runtime Cpu::rt_;
thread_local ebbrt::Cpu* ebbrt::Cpu::my_cpu_tls_;

volatile int Cpu::running_ = 0;
volatile int Cpu::created_ = 0;
volatile int Cpu::inited_ = 0;

std::mutex Cpu::init_lock_;
size_t Cpu::DefaultPhysCpus_;

ebbrt::Cpu* Cpu::Create(size_t index) {
  cpus[index].construct(index);
  numCpus_++;
  return (ebbrt::Cpu*)&cpus[index];
}

void Cpu::Init() {
  my_cpu_tls_ = this;
  set_tid(pthread_self());
}

void* Cpu::run(void* arg) {
  size_t index = (size_t)arg;
  auto n = GetPhysCpus();

  init_lock_.lock();
  auto cpu = Create(index);
  cpu->Init();
  if (size_t(*cpu) == 0) {
    // ensure clean quit on ctrl-c
    static boost::asio::signal_set signals(cpu->ctxt_.io_service_, SIGINT);
    signals.async_wait([](const boost::system::error_code& ec,
                          int signal_number) { Shutdown(); });
  }
  __sync_fetch_and_add(&created_, 1);
  init_lock_.unlock();

#ifdef __TRACE_SPAWN__
  printf("index=%zd:&cpu=%p:mine=%zd:tid=0x%zx:&cpu=%p:my_cpu_tls=%p:"
         "ctxt.index_=%zd:created\n",
         index, cpu, (size_t)GetMine(), GetMine().get_tid(), &GetMine(),
         my_cpu_tls_, size_t(cpu->ctxt_));
#endif

  while (created_ < n)
    ;

  cpu->ctxt_.Activate();
  ebbrt::event_manager->Spawn([]() {
    auto n = GetPhysCpus();
#ifdef __TRACE_SPAWN__
    printf("spawned: %zd %zd: %s:%d\n", size_t(ebbrt::Cpu::GetMine()),
           size_t(*ebbrt::active_context), __FILE__, __LINE__);
#endif
    __sync_fetch_and_add(&inited_, 1);
    while (inited_ < n)
      ;  // sync up -- for everyone to be ready to start
    if ((size_t)GetMine() == 0)
      Main();
  });
  cpu->ctxt_.Deactivate();
  __sync_fetch_and_add(&running_, 1);

  while (running_ < n)
    ;  // wait for everyone to be spawned

  // this thread should now infinitely run the event loop
  boost::asio::io_service::work work(cpu->ctxt_.io_service_);
  cpu->ctxt_.Run();
  return NULL;
}

pthread_t Cpu::EarlyInit(size_t num) {
  // lets now get all the cpus create one per physical core
  int i;
  pthread_attr_t attr;
  pthread_t rc = 0;

  if (num)
    DefaultPhysCpus_ = num;
  else {
    char* str = getenv("EBBRT_NODE_ALLOCATOR_DEFAULT_CPUS");
    DefaultPhysCpus_ = (str) ? atoi(str) : sysconf(_SC_NPROCESSORS_ONLN);
  }

  int n = GetPhysCpus();

  shutdown_.store(0);

  pthread_attr_init(&attr);

  for (i = 0; i < n; i++) {
    pthread_t tid;
    cpu_set_t cpuSet;

    CPU_ZERO(&cpuSet);
    // pin to a core, round robined over the physical cores
    CPU_SET(i, &cpuSet);
    uintptr_t id = i;
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuSet);
    if (i > 0)
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&tid, &attr, run, (void*)id) != 0) {
      perror("pthread_create");
      assert(0);
    }
    if (i == 0)
      rc = tid;
  }
  while (running_ < n)
    ;
  return rc;
}

ebbrt::Cpu* Cpu::GetByIndex(size_t index) {
  if (index > numCpus_ - 1) {
    return nullptr;
  }
  return (ebbrt::Cpu*)&(cpus[index]);
}

size_t Cpu::Count() { return numCpus_; }

void Cpu::Shutdown(void) {
  if (numCpus_ > 0) {
    // FIXME: should we not be doing this across all cpus?
    //        doing so segfaults now
    {
      int zero = 0;
      if (!shutdown_.compare_exchange_strong(zero, 1))
        return;
    }
    for (size_t i = 0; i < 1; i++) {
      printf("stopping cpu %zd\n", i);
      GetByIndex(i)->ctxt_.io_service_.stop();
    }
  }
}
void Cpu::Exit(int val) {
  Shutdown();
  exit(val);
}
}  // namespace ebbrt

void Main(void) {
  ebbrt::event_manager->Spawn([=]() {
#ifdef __TRACE_SPAWN__
    printf("spawned: %zd %zd: %s:%d\n", size_t(ebbrt::Cpu::GetMine()),
           size_t(*ebbrt::active_context), __FILE__, __LINE__);
#endif
    AppMain();
  });
}

