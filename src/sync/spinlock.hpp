#ifndef EBBRT_SYNC_SPINLOCK_HPP
#define EBBRT_SYNC_SPINLOCK_HPP

namespace ebbrt {
  class Spinlock {
  public:
    Spinlock() : lock_{ATOMIC_FLAG_INIT} {}
    inline void Lock()
    {
      while (lock_.test_and_set(std::memory_order_acquire))
        ;
    }
    inline void Unlock()
    {
      lock_.clear(std::memory_order_release);
    }
  private:
    std::atomic_flag lock_;
  };
}

#endif
