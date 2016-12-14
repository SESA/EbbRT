//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_CPU_H_
#define HOSTED_SRC_INCLUDE_EBBRT_CPU_H_

#include <mutex>
#include <pthread.h>

#include "../Nid.h"
#include "Context.h"

namespace ebbrt {
class Cpu {
 public:
  static const constexpr size_t kMaxCpus = 256;
  Cpu(size_t index) : index_(index), ctxt_(rt_) {
    // printf("%p: index=%zd index_=%zd\n", this, index, index_);
  }

  static Cpu& GetMine() { return *my_cpu_tls_; }
  static Nid GetMyNode() { return GetMine().nid(); }
  static Cpu* GetByIndex(size_t index);
  static size_t Count();
  static pthread_t EarlyInit(size_t num = 0);
  static int GetPhysCpus() { return ebbrt::Cpu::DefaultPhysCpus_; }
  static void Shutdown(void);
  static void Exit(int val);

  void Init();
  operator size_t() const { return index_; }
  Nid nid() const { return nid_; }
  void set_nid(Nid nid) { nid_ = nid; }
  void set_tid(pthread_t tid) { tid_ = tid; };
  pthread_t get_tid() { return tid_; }
  Context* get_context() { return &ctxt_; }

 private:
  static Cpu* Create(size_t index);
  static volatile int running_;
  static volatile int created_;
  static volatile int inited_;
  static void* run(void* arg);
  static Runtime rt_;
  static thread_local Cpu* my_cpu_tls_;
  static std::mutex init_lock_;
  static size_t numCpus_;
  static std::atomic<int> shutdown_;
  static size_t DefaultPhysCpus_;
  size_t index_;
  pthread_t tid_;
  Nid nid_;
  Context ctxt_;
  friend class EventManager;
};
};

#endif  // HOSTED_SRC_INCLUDE_EBBRT_CPU_H_
