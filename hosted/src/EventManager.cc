//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/EventManager.h>

#include <ebbrt/Context.h>

void ebbrt::EventManager::Spawn(MovableFunction<void()> func) {
  auto f = new MovableFunction<void()>(std::move(func));
  active_context->io_service_.dispatch([f]() {
    (*f)();
    delete f;
  });
}
