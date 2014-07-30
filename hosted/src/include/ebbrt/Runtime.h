//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_RUNTIME_H_
#define HOSTED_SRC_INCLUDE_EBBRT_RUNTIME_H_

#include <atomic>
#include <condition_variable>
#include <mutex>

namespace ebbrt {
class Runtime {
  std::mutex init_lock_;
  std::condition_variable init_cv_;
  bool initialized_;
  std::atomic<size_t> indices_;

  friend class Context;
  void Initialize();
  void DoInitialization();
 public:
  Runtime();
};
namespace runtime {
  extern char *bootcmdline;
  // dummy runtime init to keep compatability with baremetal
  void Init() __attribute__ ((unused));
}
}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_RUNTIME_H_
