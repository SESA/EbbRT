//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
#include "../Timer.h"
#include "Context.h"
#include "EventManager.h"

const constexpr ebbrt::EbbId ebbrt::Timer::static_id;

ebbrt::Timer::Timer() {}

void ebbrt::Timer::SetTimer(std::chrono::microseconds from_now) {
  EBBRT_UNIMPLEMENTED();
}

void ebbrt::Timer::StopTimer() { EBBRT_UNIMPLEMENTED(); }

void ebbrt::Timer::Start(Hook& hook, std::chrono::microseconds timeout,
                         bool repeat) {
  auto t = std::make_shared<boost::asio::deadline_timer>(
      active_context->io_service_,
      boost::posix_time::microseconds(timeout.count()));

  t->async_wait(EventManager::WrapHandler(
      [&hook, t, repeat, timeout](const boost::system::error_code& e) {
        if (e) {
          ebbrt::kabort("ASIO Error: %d\n", e.value());
        }
        if (repeat) {
          timer->Start(hook, timeout, repeat);
        }
        hook.Fire();
      }));
}

void ebbrt::Timer::Stop(Hook& hook) { EBBRT_UNIMPLEMENTED(); }
