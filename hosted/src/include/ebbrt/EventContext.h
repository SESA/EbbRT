//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_EVENTCONTEXT_H_
#define HOSTED_SRC_INCLUDE_EBBRT_EVENTCONTEXT_H_

#include <boost/coroutine/coroutine.hpp>
#include <boost/version.hpp>

namespace ebbrt {
class EventContext {
#if BOOST_VERSION > 105500
  typedef boost::coroutines::symmetric_coroutine<void>::call_type coro_type;
  typedef boost::coroutines::symmetric_coroutine<void>::yield_type caller_type;
#else
  typedef boost::coroutines::coroutine<void()> coro_type;
  typedef boost::coroutines::coroutine<void()>::caller_type caller_type;
#endif
 private:
  coro_type coro;
  caller_type* caller;

  friend class EventManager;
};
}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_EVENTCONTEXT_H_
