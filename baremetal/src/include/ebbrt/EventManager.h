//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_EVENTMANAGER_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_EVENTMANAGER_H_

#include <stack>
#include <unordered_map>

#include <ebbrt/Isr.h>
#include <ebbrt/Main.h>
#include <ebbrt/MoveLambda.h>
#include <ebbrt/Smp.h>
#include <ebbrt/Trans.h>
#include <ebbrt/VMemAllocator.h>

namespace ebbrt {

class EventManager {
 public:
  struct EventContext {
    uint64_t rbx;
    uint64_t rsp;
    uint64_t rbp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint32_t event_id;
    Pfn stack;
  };
  EventManager();

  static void Init();
  static EventManager& HandleFault(EbbId id);

  void Spawn(ebbrt::MovableFunction<void()> func);
  void SpawnLocal(ebbrt::MovableFunction<void()> func);
  void SaveContext(EventContext& context);
  void ActivateContext(const EventContext& context);
  uint8_t AllocateVector(MovableFunction<void()> func);
  uint32_t GetEventId();

 private:
  void StartProcessingEvents() __attribute__((noreturn));
  static void CallProcess(uintptr_t mgr) __attribute__((noreturn));
  void Process() __attribute__((noreturn));
  void ProcessInterrupt(int num) __attribute__((noreturn));

  Pfn AllocateStack();

  Pfn stack_;
  std::stack<Pfn> free_stacks_;
  std::stack<MovableFunction<void()>> tasks_;
  std::unordered_map<uint8_t, MovableFunction<void()>> vector_map_;
  std::atomic<uint8_t> vector_idx_;
  uint32_t active_event_id_;
  uint32_t next_event_id_;

  friend void ebbrt::idt::EventInterrupt(int num);
  friend void ebbrt::Main(ebbrt::multiboot::Information* mbi);
  friend void ebbrt::smp::SmpMain();
};

constexpr auto event_manager = EbbRef<EventManager>(kEventManagerId);
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_EVENTMANAGER_H_
