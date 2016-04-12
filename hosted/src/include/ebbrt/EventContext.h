//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_EVENTCONTEXT_H_
#define HOSTED_SRC_INCLUDE_EBBRT_EVENTCONTEXT_H_

#include <boost/coroutine/coroutine.hpp>

namespace ebbrt {
class EventContext {
 private:
  boost::coroutines::coroutine<void()> coro;
  boost::coroutines::coroutine<void()>::caller_type* caller;

  friend class EventManager;
};
}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_EVENTCONTEXT_H_
