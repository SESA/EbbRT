//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_EVENTMANAGER_H_
#define HOSTED_SRC_INCLUDE_EBBRT_EVENTMANAGER_H_

#include <ebbrt/CacheAligned.h>
#include <ebbrt/MoveLambda.h>
#include <ebbrt/StaticIds.h>
#include <ebbrt/StaticSharedEbb.h>

namespace ebbrt {

class EventManager : public StaticSharedEbb<EventManager>, public CacheAligned {
 public:
  void Spawn(ebbrt::MovableFunction<void()> func);
};

const constexpr auto event_manager = EbbRef<EventManager>(kEventManagerId);
}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_EVENTMANAGER_H_
