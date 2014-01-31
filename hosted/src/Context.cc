//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Context.h>
#include <ebbrt/ContextActivation.h>
#include <ebbrt/Runtime.h>

thread_local ebbrt::Context* ebbrt::active_context;

ebbrt::Context::Context(Runtime& runtime) : runtime_(runtime) {
  index_ = runtime.indices_.fetch_add(1, std::memory_order_relaxed);
  {
    ContextActivation activation(*this);
    runtime.Initialize();
  }
}

void ebbrt::Context::Activate() { active_context = this; }

void ebbrt::Context::Deactivate() { active_context = nullptr; }

void ebbrt::Context::Run() {
  ContextActivation active(*this);
  io_service_.run();
}
