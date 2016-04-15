//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Clock.h>
#include <ebbrt/Cpu.h>
#include <ebbrt/EbbAllocator.h>
#include <ebbrt/EventManager.h>
#include <ebbrt/MulticoreEbb.h>
#include <ebbrt/Random.h>
#include <ebbrt/RcuList.h>
#include <ebbrt/RcuTable.h>
#include <ebbrt/Rdtsc.h>
#include <ebbrt/Timer.h>

#define LIST_TEST 0
#define HASH_TEST 0

class MyClass {
 public:
  explicit MyClass(uint16_t key = 0, int value = 0) : key(key), value(value) {}

  ebbrt::RcuHListHook hook;
  uint16_t key;
  int value;
};

class Counter : public ebbrt::MulticoreEbb<Counter> {
 public:
  uint64_t count = 0;
};

class RandomNumberGenerator
    : public ebbrt::MulticoreEbb<RandomNumberGenerator> {
 public:
  std::minstd_rand generator{static_cast<unsigned int>(ebbrt::rdtsc())};
  std::uniform_int_distribution<uint16_t> dist;
};

struct MyHash {
  std::size_t operator()(const uint16_t& c) const { return c; }
};

#if LIST_TEST
ebbrt::RcuHList<MyClass, &MyClass::hook> list;
#elif HASH_TEST
ebbrt::RcuHashTable<MyClass, uint16_t, &MyClass::hook, &MyClass::key> table(13);
#endif

namespace {
ebbrt::EbbRef<Counter> counter =
    Counter::Create(ebbrt::ebb_allocator->AllocateLocal());
ebbrt::EbbRef<RandomNumberGenerator> rng =
    RandomNumberGenerator::Create(ebbrt::ebb_allocator->AllocateLocal());
ebbrt::clock::Wall::time_point tp;
alignas(64) ebbrt::SpinLock lock;
void Work() {
  auto t0 = ebbrt::clock::Wall::Now();
  const constexpr size_t iters = 1000;
  while ((ebbrt::clock::Wall::Now() - t0) < std::chrono::milliseconds(1)) {
#if HASH_TEST
    uint16_t key = rng->dist(rng->generator);
#endif
    for (size_t i = 0; i < iters; ++i) {
#if LIST_TEST
      for (auto& myc : list) {
        (void)myc;
        asm volatile("");  // prevent optimization
      }
#elif HASH_TEST
      auto p = table.find(key);
      ebbrt::kbugon(!p, "Failed to find entry in hash table\n");
      (void)const_cast<volatile MyClass*>(p)->value;
#endif
    }
    ++counter->count;
  }

  if ((ebbrt::clock::Wall::Now() - tp) > std::chrono::seconds(10)) {
    // Terminated, dump stats
    std::lock_guard<ebbrt::SpinLock> lg(lock);
    ebbrt::kprintf("%d: %llu\n", static_cast<size_t>(ebbrt::Cpu::GetMine()),
                   counter->count * iters);
  } else {
    // run again via the event loop to allow RCU and what not
    ebbrt::event_manager->SpawnLocal(Work, /* force_async = */ true);
  }
}
}  // namespace

struct alignas(64) val {
  uint8_t shift;
  bool resizing;
} value;

void AppMain() {
#if LIST_TEST
  ebbrt::timer->Start(std::chrono::milliseconds(50),
                      []() {
                        // insert
                        kassert(ebbrt::Cpu::GetMine() == 0);
                        list.push_front(*new MyClass(0));
                        std::atomic_thread_fence(std::memory_order_release);
                      },
                      /* repeat = */ true);
  ebbrt::timer->Start(std::chrono::milliseconds(50),
                      []() {
                        kassert(ebbrt::Cpu::GetMine() == 0);
                        // no need to synchronize since this runs on the same
                        // core as the inserter
                        if (!list.empty()) {
                          auto ptr = &*list.begin();
                          list.erase(*ptr);
                          std::atomic_thread_fence(std::memory_order_release);
                          ebbrt::event_manager->DoRcu([ptr]() { delete ptr; });
                        }
                      },
                      /* repeat = */ true);
#elif HASH_TEST
  for (uint32_t i = 0; i <= UINT16_MAX; ++i) {
    table.insert(*new MyClass(static_cast<uint16_t>(i)));
    std::atomic_signal_fence(std::memory_order_release);
  }
  value.shift = 14;
  value.resizing = false;
  ebbrt::timer->Start(std::chrono::milliseconds(500),
                      []() {
                        kassert(ebbrt::Cpu::GetMine() == 0);
                        auto shift = value.shift;
                        value.shift = shift == 13 ? 14 : 13;
                        assert(!value.resizing);
                        table.resize(shift).Then([](ebbrt::Future<void> f) {
                          f.Get();
                          value.resizing = false;
                        });
                      },
                      /* repeat = */ true);
#endif
  ebbrt::kprintf("Start\n");
  tp = ebbrt::clock::Wall::Now();
  for (size_t i = 1; i < ebbrt::Cpu::Count(); ++i) {
    ebbrt::event_manager->SpawnRemote(Work, i);
  }
  Work();
}
