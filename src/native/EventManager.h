//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_EVENTMANAGER_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_EVENTMANAGER_H_

#include <list>
#include <mutex>
#include <queue>
#include <stack>
#include <unordered_map>

#include <boost/container/flat_map.hpp>
#include <boost/utility.hpp>

#include "../MoveLambda.h"
#include "../Timer.h"
#include "Cpu.h"
#include "Isr.h"
#include "Main.h"
#include "Smp.h"
#include "Trans.h"
#include "VMemAllocator.h"

namespace ebbrt {

class EventManager : Timer::Hook {
  typedef boost::container::flat_map<size_t, ebbrt::EventManager*> RepMap;

 public:
  struct EventContext {
    EventContext();
    EventContext(uint32_t event_id, Pfn stack);
    EventContext(const EventContext&) = delete;
    EventContext& operator=(const EventContext&) = delete;
    EventContext(EventContext&&) = default;
    EventContext& operator=(EventContext&&) = default;
    uint64_t rbx;
    uint64_t rsp;
    uint64_t rbp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint32_t event_id;
    Pfn stack;
    std::unique_ptr<std::unordered_map<__gthread_key_t, void*>> tls;
    size_t cpu;
    size_t generation;
  };
  class IdleCallback : boost::noncopyable {
   public:
    template <typename F>
    explicit IdleCallback(F&& f) : f_(std::forward<F>(f)), started_(false) {}

    void Start();
    void Stop();

   private:
    std::function<void()> f_;
    bool started_;
  };

  explicit EventManager(const RepMap& rm);

  static void Init();
  static EventManager& HandleFault(EbbId id);

  void Spawn(ebbrt::MovableFunction<void()> func, bool force_async = false);
  void SpawnLocal(ebbrt::MovableFunction<void()> func,
                  bool force_async = false);
  void SpawnRemote(ebbrt::MovableFunction<void()> func, size_t cpu);
  void SaveContext(EventContext& context);
  void ActivateContext(EventContext&& context);
  void ActivateContextSync(EventContext&& context);
  uint8_t AllocateVector(MovableFunction<void()> func);
  uint32_t GetEventId();
  size_t QueueLength();
  std::unordered_map<__gthread_key_t, void*>& GetTlsMap();
  void DoRcu(MovableFunction<void()> func);
  void Fire() override;
  void PreAllocateStacks(size_t num);

 private:
  template <typename F> void InvokeFunction(F&& f);
  void AddRemoteTask(MovableFunction<void()> func);
  void StartProcessingEvents()
      __attribute__((noreturn, no_instrument_function));
  static void CallProcess(uintptr_t mgr)
      __attribute__((noreturn, no_instrument_function));
  static void CallSync(uintptr_t mgr)
      __attribute__((noreturn, no_instrument_function));
  void Process() __attribute__((noreturn, no_instrument_function));
  void ProcessInterrupt(int num)
      __attribute__((noreturn, no_instrument_function));
  Pfn AllocateStack();
  void FreeStack(Pfn pfn);
  void PassToken();
  void ReceiveToken();
  void CheckGeneration();
  void StartTimer();

  const RepMap& reps_;
  std::stack<Pfn> free_stacks_;
  std::list<MovableFunction<void()>> tasks_;
  uint32_t next_event_id_;
  EventContext active_event_context_;
  std::stack<EventContext> sync_contexts_;
  MovableFunction<void()> sync_spawn_fn_;
  std::function<void()>* idle_callback_ = nullptr;
  size_t generation_ = 0;
  std::array<size_t, 2> generation_count_ = {{0}};
  size_t pending_generation_ = 0;
  std::queue<MovableFunction<void()>> prev_rcu_tasks_;
  std::queue<MovableFunction<void()>> curr_rcu_tasks_;
  size_t allocate_stack_counter_ = 0;

  struct RemoteData : CacheAligned {
    ebbrt::SpinLock lock;
    std::list<MovableFunction<void()>> tasks;
  } remote_;

  friend void ebbrt::idt::EventInterrupt(int num);
  friend void ebbrt::Main(ebbrt::multiboot::Information* mbi);
  friend void ebbrt::smp::SmpMain();
};

constexpr auto event_manager = EbbRef<EventManager>(kEventManagerId);
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_EVENTMANAGER_H_
