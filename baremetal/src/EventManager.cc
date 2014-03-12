//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/EventManager.h>

#include <unordered_map>

#include <boost/container/flat_map.hpp>

#include <ebbrt/Cpu.h>
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/PageAllocator.h>
#include <ebbrt/Trace.h>
#include <ebbrt/VMem.h>

void ebbrt::EventManager::Init() {
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

extern "C" __attribute__((noreturn)) void SwitchStack(uintptr_t first_param,
                                                      uintptr_t stack,
                                                      void (*func)(uintptr_t));

void ebbrt::EventManager::StartProcessingEvents() {
  auto stack_top = (active_event_context_.stack + kStackPages).ToAddr();
  Cpu::GetMine().SetEventStack(stack_top);
  SwitchStack(reinterpret_cast<uintptr_t>(this), stack_top, CallProcess);
}

void ebbrt::EventManager::CallProcess(uintptr_t mgr) {
  auto pmgr = reinterpret_cast<EventManager*>(mgr);
  pmgr->Process();
}

namespace {
void InvokeFunction(ebbrt::MovableFunction<void()>& f) {
  try {
    f();
  }
  catch (std::exception& e) {
    ebbrt::kabort("Unhandled exception caught: %s\n", e.what());
  }
  catch (...) {
    ebbrt::kabort("Unhandled exception caught!\n");
  }
}
}  // namespace

void ebbrt::EventManager::Process() {
  // process an interrupt without halting
  // the sti instruction starts processing interrupts *after* the next
  // instruction is executed (to allow for a halt for example). The nop gives us
  // a one instruction window to process an interrupt (before the cli)
  while (1) {
    asm volatile("sti;"
                 "nop;"
                 "cli;");
    // If an interrupt was processed then we would not reach this code (the
    // interrupt does not return here but instead to the top of this function)

    tryCnt_++;
    // to ensure that we don't starve we alternate between the local
    // and remote queues
    if ((tryCnt_ & 0x1) == 0) {
      if (!tasks_.empty()) {
        auto f = std::move(tasks_.top());
        tasks_.pop();
        InvokeFunction(f);
      } else {
        // When the local task set is empty we can safely halt however for
        // improved latency you may want a grace period before halting
        // However given powerboast this maynot be an easy decision.
        // This is safe since remote side will latch an interrupt in hardware
        // and will ensure that interrupts either execute before halt or cause
        // halt to return and then execute
        rtsk_lock_.lock();
        bool empty = rtasks_.empty();
        rtsk_lock_.unlock();
        if (empty) {
          asm volatile("sti;"
                       "hlt;");
        }
      }
    } else {
      rtsk_lock_.lock();
      if (!rtasks_.empty()) {
        auto f = std::move(rtasks_.top());
        rtasks_.pop();
        rtsk_lock_.unlock();
        InvokeFunction(f);
      } else {
        rtsk_lock_.unlock();
      }
    }
  }
}

ebbrt::Pfn ebbrt::EventManager::AllocateStack() {
  if (!free_stacks_.empty()) {
    auto ret = free_stacks_.top();
    free_stacks_.pop();
    return ret;
  }
  auto fault_handler = new EventStackFaultHandler;
  return vmem_allocator->Alloc(
      kStackPages, std::unique_ptr<EventStackFaultHandler>(fault_handler));
}

static_assert(ebbrt::Cpu::kMaxCpus <= 256, "adjust event id calculation");

ebbrt::EventManager::EventManager(const RepMap& rm)
    : reps_(rm), vector_idx_(32), next_event_id_(Cpu::GetMine() << 24),
      active_event_context_(next_event_id_++, AllocateStack()), tryCnt_(0) {}

void ebbrt::EventManager::Spawn(MovableFunction<void()> func) {
  SpawnLocal(std::move(func));
}

void ebbrt::EventManager::SpawnLocal(MovableFunction<void()> func) {
  tasks_.emplace(std::move(func));
}

void ebbrt::EventManager::addRemoteTask(MovableFunction<void()> func) {
  std::lock_guard<std::mutex> lock(rtsk_lock_);
  rtasks_.emplace(std::move(func));
}

void ebbrt::EventManager::SpawnRemote(MovableFunction<void()> func,
                                      size_t cpu) {
  // FIXME: we probably should make this work even in the case the the rep for
  // cpu is not created yet but for the moment we are going to let
  // this throw an error in this case!
  // kassert(reps_[cpu]);
  auto rep = reps_.find(cpu);
  kassert(rep != reps_.end());
  rep->second->addRemoteTask(func);
  // We might want to do a queue of ipi's that can
  // be cancelled if the work was acked via shared memory due to
  // natural polling on the remote processor
  //  reps_[cpu].ipi(...);
}

extern "C" void
SaveContextAndSwitch(uintptr_t first_param, uintptr_t stack,
                     void (*func)(uintptr_t),
                     ebbrt::EventManager::EventContext& context);

void ebbrt::EventManager::SaveContext(EventContext& context) {
  context = std::move(active_event_context_);
  auto stack = AllocateStack();
  auto stack_top = (stack + kStackPages).ToAddr();
  Cpu::GetMine().SetEventStack(stack_top);
  active_event_context_.~EventContext();
  new (&active_event_context_) EventContext(next_event_id_++, stack);
  SaveContextAndSwitch(reinterpret_cast<uintptr_t>(this), stack_top,
                       CallProcess, context);
}

extern "C" void
ActivateContextAndReturn(const ebbrt::EventManager::EventContext& context);

void ebbrt::EventManager::ActivateContext(EventContext&& context) {
  if (context.cpu == Cpu::GetMine()) {
    // if we are activting on the same core as the context was
    // saved on then we need not syncronize and know that we
    // can Spawn Locally
    SpawnLocal(MoveBind([this](EventContext c) {
                          // ActivatePrivate(std::move(c))
                          free_stacks_.push(active_event_context_.stack);
                          auto stack_top = (c.stack + kStackPages).ToAddr();
                          Cpu::GetMine().SetEventStack(stack_top);
                          active_event_context_ = std::move(c);
                          ActivateContextAndReturn(active_event_context_);
                        },
                        std::move(context)));
  } else {
    auto rep_iter = reps_.find(context.cpu);
    auto rep = rep_iter->second;
    SpawnRemote(MoveBind([rep](EventContext c) {
                           // event_manager->ActivatePrivate(std::move(c))
                           rep->free_stacks_.push(
                               rep->active_event_context_.stack);
                           auto stack_top = (c.stack + kStackPages).ToAddr();
                           Cpu::GetMine().SetEventStack(stack_top);
                           rep->active_event_context_ = std::move(c);
                           ActivateContextAndReturn(rep->active_event_context_);
                         },
                         std::move(context)),
                context.cpu);  // SpawnRemote Argument 2
  }
}

uint8_t ebbrt::EventManager::AllocateVector(MovableFunction<void()> func) {
  auto vec = vector_idx_.fetch_add(1, std::memory_order_relaxed);
  vector_map_.emplace(vec, std::move(func));
  return vec;
}

void ebbrt::EventManager::ProcessInterrupt(int num) {
  apic::Eoi();
  auto it = vector_map_.find(num);
  if (it != vector_map_.end()) {
    auto& f = it->second;
    InvokeFunction(f);
  }
  Process();
}

uint32_t ebbrt::EventManager::GetEventId() {
  return active_event_context_.event_id;
}

std::unordered_map<__gthread_key_t, void*>& ebbrt::EventManager::GetTlsMap() {
  return active_event_context_.tls;
}
