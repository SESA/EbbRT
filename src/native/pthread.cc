#include "pthread.h"

#include <ebbrt/EventManager.h>
#include <ebbrt/Future.h>
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/SpinBarrier.h>
#include <ebbrt/UniqueIOBuf.h>
#include <ebbrt/native/Cpu.h>

static std::vector<ebbrt::Future<uint32_t>> vecfut_;
static std::unordered_map<uint32_t, ebbrt::Promise<uint32_t>> mmap_;
static uint64_t tid_;

ebbrt::SpinLock spinlock_;

void pthread_attr_init(pthread_attr_t* attr) {
  vecfut_.clear();
  mmap_.clear();
  tid_ = 0;
}

int pthread_create(pthread_t* th, pthread_attr_t* attr, void* (*func)(void*),
                   void* arg) {

  *th = reinterpret_cast<pthread_t>(tid_);
  size_t tid = reinterpret_cast<size_t>(tid_);

  ebbrt::Promise<uint32_t> promise;
  bool inserted;
  auto f = promise.GetFuture();
  vecfut_.push_back(std::move(f));
  std::tie(std::ignore, inserted) = mmap_.emplace(tid, std::move(promise));

  ebbrt::event_manager->SpawnRemote(
      [func, arg, tid]() {
        func((void*)arg);

        std::lock_guard<ebbrt::SpinLock> l(spinlock_);
        {
          auto it = mmap_.find(tid);
          assert(it != mmap_.end());
          it->second.SetValue(tid);
          mmap_.erase(it);
        }
      },
      tid);

  tid_++;

  return 0;
}

int pthread_join(pthread_t threadn, void** value_ptr) {
  size_t tid = static_cast<size_t>(threadn);
  vecfut_[tid].Block();

  return 0;
}
