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
#include <ebbrt/VMem.h>

typedef boost::container::flat_map<size_t, ebbrt::EventManager*> RepMap;

void ebbrt::EventManager::Init() {
  local_id_map->Insert(std::make_pair(kEventManagerId, RepMap()));
}

ebbrt::EventManager& ebbrt::EventManager::HandleFault(EbbId id) {
  kassert(id == kEventManagerId);
  {
    // Acquire read only to find rep
    LocalIdMap::ConstAccessor accessor;
    auto found = local_id_map->Find(accessor, id);
    kassert(found);
    auto rep_map = boost::any_cast<RepMap>(accessor->second);
    auto it = rep_map.find(Cpu::GetMine());
    if (it != rep_map.end()) {
      EbbRef<EventManager>::CacheRef(id, *it->second);
      return *it->second;
    }
  }
  // OK we must construct a rep
  auto rep = new EventManager;
  LocalIdMap::Accessor accessor;
  auto found = local_id_map->Find(accessor, id);
  kassert(found);
  auto rep_map = boost::any_cast<RepMap>(accessor->second);
  rep_map[Cpu::GetMine()] = rep;
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
  auto stack_top = (stack_ + kStackPages).ToAddr();
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
process:
  asm volatile("sti;"
               "nop;"
               "cli;");
  // If an interrupt was processed then we would not reach this code (the
  // interrupt does not return here but instead to the top of this function)

  if (!tasks_.empty()) {
    auto f = std::move(tasks_.top());
    tasks_.pop();
    active_event_id_ = next_event_id_++;
    InvokeFunction(f);
    // if we had a task to execute, then we go to the top again
    goto process;
  }

  // We only reach here if we had no interrupts to process or tasks to run. We
  // halt until an interrupt wakes us
  asm volatile("sti;"
               "hlt;");
  kabort("Woke up from halt?!?!");
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

ebbrt::EventManager::EventManager()
    : vector_idx_(32), next_event_id_(Cpu::GetMine() << 24) {
  stack_ = AllocateStack();
}

void ebbrt::EventManager::Spawn(MovableFunction<void()> func) {
  SpawnLocal(std::move(func));
}

void ebbrt::EventManager::SpawnLocal(MovableFunction<void()> func) {
  tasks_.emplace(std::move(func));
}

extern "C" void
SaveContextAndSwitch(uintptr_t first_param, uintptr_t stack,
                     void (*func)(uintptr_t),
                     ebbrt::EventManager::EventContext& context);

void ebbrt::EventManager::SaveContext(EventContext& context) {
  context.stack = stack_;
  context.event_id = active_event_id_;
  stack_ = AllocateStack();
  auto stack_top = (stack_ + kStackPages).ToAddr();
  Cpu::GetMine().SetEventStack(stack_top);
  SaveContextAndSwitch(reinterpret_cast<uintptr_t>(this), stack_top,
                       CallProcess, context);
}

extern "C" void
ActivateContextAndReturn(const ebbrt::EventManager::EventContext& context);

void ebbrt::EventManager::ActivateContext(const EventContext& context) {
  SpawnLocal([this, context]() {
    free_stacks_.push(stack_);
    stack_ = context.stack;
    active_event_id_ = context.event_id;
    auto stack_top = (context.stack + kStackPages).ToAddr();
    Cpu::GetMine().SetEventStack(stack_top);
    ActivateContextAndReturn(context);
  });
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
    active_event_id_ = next_event_id_++;
    InvokeFunction(f);
  }
  Process();
}

uint32_t ebbrt::EventManager::GetEventId() { return active_event_id_; }
