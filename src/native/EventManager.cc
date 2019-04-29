//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include "EventManager.h"

#include <unordered_map>

#include <boost/container/flat_map.hpp>

#include "../Compiler.h"
#include "Cpu.h"
#include "LocalIdMap.h"
#include "PageAllocator.h"
#include "RcuTable.h"
#include "Trace.h"
#include "VMem.h"

namespace {
struct InterruptHandler {
  ebbrt::RcuHListHook hook;
  ebbrt::MovableFunction<void()> func;
  uint8_t interrupt;
};

struct vec_data_t : public ebbrt::CacheAligned {
  ebbrt::SpinLock lock;
  uint8_t idx{34};
  ebbrt::RcuHashTable<InterruptHandler, uint8_t, &InterruptHandler::hook,
                      &InterruptHandler::interrupt>
      map{8};
};

ebbrt::ExplicitlyConstructed<vec_data_t> vec_data;
}  // namespace

void ebbrt::EventManager::Init() {
  vec_data.construct();
  local_id_map->Insert(std::make_pair(kEventManagerId, RepMap()));
}

ebbrt::EventManager& ebbrt::EventManager::HandleFault(EbbId id) {
  kassert(id == kEventManagerId);
  const RepMap* rep_map;
  {
    // Acquire read only to find rep
    LocalIdMap::ConstAccessor accessor;
    auto found = local_id_map->Find(accessor, id);
    kassert(found);
    rep_map = boost::any_cast<RepMap>(&accessor->second);
    auto it = rep_map->find(Cpu::GetMine());
    if (it != rep_map->end()) {
      EbbRef<EventManager>::CacheRef(id, *it->second);
      return *it->second;
    }
  }

  auto rep = new EventManager(*rep_map);
  // OK we must construct a rep
  {
    LocalIdMap::Accessor accessor;
    auto found = local_id_map->Find(accessor, id);
    kassert(found);
    auto writable_rep_map = boost::any_cast<RepMap>(&accessor->second);
    writable_rep_map->emplace(Cpu::GetMine(), rep);
  }
  EbbRef<EventManager>::CacheRef(id, *rep);
  return *rep;
}

namespace {
const constexpr size_t kStackPages = 2048;  // 8 MB stack
}

class EventStackFaultHandler : public ebbrt::VMemAllocator::PageFaultHandler {
 public:
  EventStackFaultHandler() = default;
  EventStackFaultHandler(const EventStackFaultHandler&) = delete;
  EventStackFaultHandler& operator=(const EventStackFaultHandler&) = delete;
  ~EventStackFaultHandler() {
    ebbrt::kbugon(!mappings_.empty(), "Free stack pages!\n");
  }
  void HandleFault(ebbrt::idt::ExceptionFrame* ef,
                   uintptr_t faulted_address) override {
    auto page = ebbrt::Pfn::Down(faulted_address);
    auto it = mappings_.find(page);
    if (it == mappings_.end()) {
      ebbrt::kbugon(mappings_.size() + 1 >= kStackPages, "Stack overflow!\n");
      auto backing_page = ebbrt::page_allocator->Alloc();
      ebbrt::kbugon(backing_page == ebbrt::Pfn::None(),
                    "Failed to allocate page for stack\n");
      ebbrt::vmem::MapMemory(page, backing_page);
      mappings_[page] = backing_page;
    } else {
      ebbrt::vmem::MapMemory(page, it->second);
    }
  }

 private:
  std::unordered_map<ebbrt::Pfn, ebbrt::Pfn> mappings_;
};

extern "C" __attribute__((noreturn)) void
SwitchStack(uintptr_t first_param, uintptr_t stack, void (*func)(uintptr_t));

void ebbrt::EventManager::StartProcessingEvents() {
  auto stack_top = (active_event_context_.stack + kStackPages).ToAddr();
  SwitchStack(reinterpret_cast<uintptr_t>(this), stack_top, CallProcess);
}

void ebbrt::EventManager::CallProcess(uintptr_t mgr) {
  auto pmgr = reinterpret_cast<EventManager*>(mgr);
  pmgr->Process();
}

template <typename F> void ebbrt::EventManager::InvokeFunction(F&& f) {
  try {
    auto generation = generation_;
    active_event_context_.generation = generation;
    ++generation_count_[generation % 2];
    f();
    --generation_count_[generation % 2];
  } catch (std::exception& e) {
    ebbrt::kabort("Unhandled exception caught: %s\n", e.what());
  } catch (...) {
    ebbrt::kabort("Unhandled exception caught!\n");
  }
}

void ebbrt::EventManager::Process() {
  auto stack_top = (active_event_context_.stack + kStackPages).ToAddr();
  Cpu::GetMine().SetEventStack(stack_top);
// process an interrupt without halting
// the sti instruction starts processing interrupts *after* the next
// instruction is executed (to allow for a halt for example). The nop gives us
// a one instruction window to process an interrupt (before the cli)
process:
  asm volatile("sti;"
               "nop;"
               "cli;");
  // If an interrupt was processed then we would not reach this code (the
  // interrupt does not return here but instead to the top of this function)

  if (!tasks_.empty()) {
    auto f = std::move(tasks_.front());
    tasks_.pop_front();
    InvokeFunction(f);
    // if we had a task to execute, then we go to the top again
    goto process;
  }

  if (idle_callback_) {
    InvokeFunction(*idle_callback_);
    goto process;
  }

  asm volatile("sti;"
               "hlt;");
  kabort("Woke up from halt?!?!");
}

ebbrt::Pfn ebbrt::EventManager::AllocateStack() {
  if (likely(!free_stacks_.empty())) {
    auto ret = free_stacks_.top();
    free_stacks_.pop();
    return ret;
  }
  allocate_stack_counter_++;
  if (allocate_stack_counter_ % 10 == 0) {
    ebbrt::kprintf("EventManager: core %d has allocated %d stacks\n",
                   (size_t)Cpu::GetMine(), allocate_stack_counter_);
  }

  auto fault_handler = new EventStackFaultHandler;
  return vmem_allocator->Alloc(
      kStackPages, std::unique_ptr<EventStackFaultHandler>(fault_handler));
}

void ebbrt::EventManager::FreeStack(Pfn stack) { free_stacks_.push(stack); }

static_assert(ebbrt::Cpu::kMaxCpus <= 256, "adjust event id calculation");

ebbrt::EventManager::EventManager(const RepMap& rm)
    : reps_(rm), next_event_id_(Cpu::GetMine() << 24),
      active_event_context_(next_event_id_++, AllocateStack()) {}

void ebbrt::EventManager::Spawn(MovableFunction<void()> func,
                                bool force_async) {
  SpawnLocal(std::move(func), force_async);
}

extern "C" void
ActivateContextAndReturn(const ebbrt::EventManager::EventContext& context)
    __attribute__((noreturn));

extern "C" void
SaveContextAndSwitch(uintptr_t first_param, uintptr_t stack,
                     void (*func)(uintptr_t),
                     ebbrt::EventManager::EventContext& context);

void ebbrt::EventManager::CallSync(uintptr_t mgr) {
  auto pmgr = reinterpret_cast<EventManager*>(mgr);
  // TRICKY: This function does not terminate, so we must ensure we
  // destroy any local variables (hence the local scope here)
  {
    auto fn = std::move(pmgr->sync_spawn_fn_);
    pmgr->InvokeFunction(fn);
  }
  // In the case that the event blocked, it will only be reactivated on a
  // "fresh" event. Therefore if the sync_contexts_ stack is empty, we just go
  // back to the event loop
  if (unlikely(pmgr->sync_contexts_.empty())) {
    pmgr->Process();
  } else {
    // save this stack
    pmgr->FreeStack(pmgr->active_event_context_.stack);
    // reload previous stack
    auto& prev_context = pmgr->sync_contexts_.top();
    pmgr->active_event_context_ = std::move(prev_context);
    pmgr->sync_contexts_.pop();
    ActivateContextAndReturn(pmgr->active_event_context_);
  }
}

void ebbrt::EventManager::SpawnLocal(MovableFunction<void()> func,
                                     bool force_async) {
  if (unlikely(force_async)) {
    tasks_.emplace_back(std::move(func));
  } else {
    sync_spawn_fn_ = std::move(func);

    // put current context on the stack
    sync_contexts_.emplace(std::move(active_event_context_));

    // create new active context
    auto stack = AllocateStack();
    auto stack_top = (stack + kStackPages).ToAddr();
    active_event_context_ = EventContext(next_event_id_++, stack);

    // save the current context to the stack and switches
    SaveContextAndSwitch(reinterpret_cast<uintptr_t>(this), stack_top, CallSync,
                         sync_contexts_.top());
  }
}

void ebbrt::EventManager::AddRemoteTask(MovableFunction<void()> func) {
  std::lock_guard<ebbrt::SpinLock> lock(remote_.lock);
  remote_.tasks.emplace_back(std::move(func));
}

void ebbrt::EventManager::SpawnRemote(MovableFunction<void()> func,
                                      size_t cpu) {
  auto rep = reps_.find(cpu);
  kassert(rep != reps_.end());
  rep->second->AddRemoteTask(std::move(func));
  // We might want to do a queue of ipi's that can
  // be cancelled if the work was acked via shared memory due to
  // natural polling on the remote processor
  auto c = Cpu::GetByIndex(cpu);
  kassert(c != nullptr);
  auto apic_id = c->apic_id();
  apic::Ipi(apic_id, 32);
}

extern "C" void SaveContextAndActivate(
    ebbrt::EventManager::EventContext& save_context,
    const ebbrt::EventManager::EventContext& activate_context);

void ebbrt::EventManager::SaveContext(EventContext& context) {
  context = std::move(active_event_context_);
  if (sync_contexts_.empty()) {
    auto stack = AllocateStack();
    auto stack_top = (stack + kStackPages).ToAddr();
    active_event_context_.~EventContext();
    new (&active_event_context_) EventContext(next_event_id_++, stack);
    SaveContextAndSwitch(reinterpret_cast<uintptr_t>(this), stack_top,
                         CallProcess, context);
  } else {
    auto& prev_context = sync_contexts_.top();
    active_event_context_ = std::move(prev_context);
    sync_contexts_.pop();
    SaveContextAndActivate(context, active_event_context_);
  }
}

void ebbrt::EventManager::PreAllocateStacks(size_t num) {
  while(num--){
    allocate_stack_counter_++;
    if (allocate_stack_counter_ % 10 == 0) {
      ebbrt::kprintf("EventManager: core %d has allocated %d stacks\n",
                     (size_t)Cpu::GetMine(), allocate_stack_counter_);
    }
    auto fault_handler = new EventStackFaultHandler;
    auto s = vmem_allocator->Alloc(
        kStackPages, std::unique_ptr<EventStackFaultHandler>(fault_handler));
    FreeStack(s);
  }
}

void ebbrt::EventManager::ActivateContext(EventContext&& context) {
  if (context.cpu == Cpu::GetMine()) {
    // if we are activting on the same core as the context was
    // saved on then we need not syncronize and know that we
    // can Spawn Locally
    SpawnLocal([ this, c = std::move(context) ]() mutable {
      // ActivatePrivate(std::move(c))
      FreeStack(active_event_context_.stack);
      // We need to switch the event stack because we only
      // set it at the top of process
      auto stack_top = (c.stack + kStackPages).ToAddr();
      Cpu::GetMine().SetEventStack(stack_top);
      active_event_context_ = std::move(c);
      ActivateContextAndReturn(active_event_context_);
    },
               /* force_async = */ true);
  } else {
    auto cpu = context.cpu;
    auto rep_iter = reps_.find(cpu);
    auto rep = rep_iter->second;
    SpawnRemote([ rep, c = std::move(context) ]() mutable {
      // event_manager->ActivatePrivate(std::move(c))
      rep->FreeStack(rep->active_event_context_.stack);
      auto stack_top = (c.stack + kStackPages).ToAddr();
      Cpu::GetMine().SetEventStack(stack_top);
      rep->active_event_context_ = std::move(c);
      ActivateContextAndReturn(rep->active_event_context_);
    },
                cpu);  // SpawnRemote Argument 2
  }
}

void ebbrt::EventManager::ActivateContextSync(EventContext&& context) {
  kassert(context.cpu == Cpu::GetMine());
  sync_contexts_.emplace(std::move(active_event_context_));
  active_event_context_ = std::move(context);
  SaveContextAndActivate(sync_contexts_.top(), active_event_context_);
}

uint8_t ebbrt::EventManager::AllocateVector(MovableFunction<void()> func) {
  std::lock_guard<ebbrt::SpinLock> lock(vec_data->lock);
  auto vec = vec_data->idx++;
  auto handler = new InterruptHandler();
  handler->interrupt = vec;
  handler->func = std::move(func);
  vec_data->map.insert(*handler);
  return vec;
}

void ebbrt::EventManager::ProcessInterrupt(int num) {
  apic::Eoi();
  if (num == 32) {
    // pull all remote tasks onto our queue
    std::lock_guard<ebbrt::SpinLock> l(remote_.lock);
    tasks_.splice(tasks_.end(), std::move(remote_.tasks));
  } else if (num == 33) {
    ReceiveToken();
  } else {
    auto ih = vec_data->map.find(num);
    kassert(ih != nullptr);
    auto& f = ih->func;
    InvokeFunction(f);
  }
  Process();
}

uint32_t ebbrt::EventManager::GetEventId() {
  return active_event_context_.event_id;
}

size_t ebbrt::EventManager::QueueLength() {
  return tasks_.size();
}

std::unordered_map<__gthread_key_t, void*>& ebbrt::EventManager::GetTlsMap() {
  if (unlikely(!active_event_context_.tls)) {
    active_event_context_.tls.reset(
        new std::unordered_map<__gthread_key_t, void*>());
  }
  return *active_event_context_.tls;
}

void ebbrt::EventManager::IdleCallback::Start() {
  if (!started_) {
    kbugon(event_manager->idle_callback_ != nullptr,
           "Multiple idle callbacks!\n");
    event_manager->idle_callback_ = &f_;
    started_ = true;
  }
}

void ebbrt::EventManager::IdleCallback::Stop() {
  if (started_) {
    event_manager->idle_callback_ = nullptr;
    started_ = false;
  }
}

ebbrt::EventManager::EventContext::EventContext() : cpu(Cpu::GetMine()) {}

ebbrt::EventManager::EventContext::EventContext(uint32_t event_id, Pfn stack)
    : event_id(event_id), stack(stack), cpu(Cpu::GetMine()) {}

void ebbrt::EventManager::PassToken() {
  size_t my_cpu_index = Cpu::GetMine();
  if (Cpu::Count() > 1) {
    auto next_cpu_index = (my_cpu_index + 1) % Cpu::Count();
    auto next_cpu = Cpu::GetByIndex(next_cpu_index);
    kassert(next_cpu != nullptr);
    apic::Ipi(next_cpu->apic_id(), 33);
  } else {
    ReceiveToken();
  }
}

void ebbrt::EventManager::ReceiveToken() {
  pending_generation_ = generation_++;

  StartTimer();
}

// Check Generation
void ebbrt::EventManager::Fire() {
  if (generation_count_[pending_generation_ % 2] == 0) {
    // generation complete
    PassToken();
    // temporarily store tasks that have now lived at least one entire
    // generation (can be invoked)
    auto tasks = std::move(prev_rcu_tasks_);
    // current tasks stored
    prev_rcu_tasks_ = std::move(curr_rcu_tasks_);
    while (!tasks.empty()) {
      auto& task = tasks.front();
      SpawnLocal(std::move(task));
      tasks.pop();
    }
  } else {
    StartTimer();
  }
}

void ebbrt::EventManager::StartTimer() {
  timer->Start(*this, std::chrono::milliseconds(1),
               /* repeat = */ false);
}

void ebbrt::EventManager::DoRcu(MovableFunction<void()> func) {
  curr_rcu_tasks_.emplace(std::move(func));
}
