//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_EVENTMANAGER_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_EVENTMANAGER_H_

#include <list>
#include <mutex>
#include <stack>
#include <unordered_map>

#include <boost/container/flat_map.hpp>
#include <boost/utility.hpp>

#include <ebbrt/Isr.h>
#include <ebbrt/Main.h>
#include <ebbrt/MoveLambda.h>
#include <ebbrt/Smp.h>
#include <ebbrt/Trans.h>
#include <ebbrt/VMemAllocator.h>
#include <ebbrt/Cpu.h>

namespace ebbrt {

class EventManager {
  typedef boost::container::flat_map<size_t, ebbrt::EventManager*> RepMap;

 public:
  struct EventContext {
    EventContext() : cpu(Cpu::GetMine()) {}
    EventContext(uint32_t event_id, Pfn stack)
        : event_id(event_id), stack(stack), cpu(Cpu::GetMine()) {}
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
  };
  class IdleCallback : boost::noncopyable {
   public:
    template <typename F>
    explicit IdleCallback(F&& f)
        : f_(std::forward<F>(f)), started_(false) {}

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
  uint8_t AllocateVector(MovableFunction<void()> func);
  uint32_t GetEventId();
  std::unordered_map<__gthread_key_t, void*>& GetTlsMap();

 private:
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

  const RepMap& reps_;
  std::stack<Pfn> free_stacks_;
  std::list<MovableFunction<void()>> tasks_;
  std::unordered_map<uint8_t, MovableFunction<void()>> vector_map_;
  std::atomic<uint8_t> vector_idx_;
  uint32_t next_event_id_;
  EventContext active_event_context_;
  std::stack<EventContext> sync_contexts_;
  MovableFunction<void()> sync_spawn_fn_;
  std::function<void()>* idle_callback_;

  struct RemoteData : CacheAligned {
    std::mutex lock;
    std::list<MovableFunction<void()>> tasks;
  } remote_;

  friend void ebbrt::idt::EventInterrupt(int num);
  friend void ebbrt::Main(ebbrt::multiboot::Information* mbi);
  friend void ebbrt::smp::SmpMain();
};

constexpr auto event_manager = EbbRef<EventManager>(kEventManagerId);
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_EVENTMANAGER_H_
