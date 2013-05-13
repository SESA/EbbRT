#ifndef EBBRT_EBB_DISTRIBUTEDROOT_HPP
#define EBBRT_EBB_DISTRIBUTEDROOT_HPP

#include "ebb/ebb.hpp"

namespace ebbrt {
  template <typename T>
  class DistributedRoot : public EbbRoot {
  public:
    virtual bool PreCall(Args* args, ptrdiff_t fnum,
                         lrt::trans::FuncRet* fret)
    {
      auto it = reps_.find(get_location());
      T* ref;
      if (it == reps_.end()) {
        // No rep for this location
        ref = new T();
        ebb_allocator->CacheRep(memory_allocator, ref);
        reps_[get_location()] = ref;
      } else {
        ebb_allocator->CacheRep(memory_allocator, it->second);
        ref = it->second;
      }
      *reinterpret_cast<EbbRep**>(args) = ref;
      // rep is a pointer to pointer to array 256 of pointer to
      // function returning void
      void (*(**rep)[256])() = reinterpret_cast<void (*(**)[256])()>(ref);
      fret->func = (**rep)[fnum];
      return true;
    }
    virtual void* PostCall(void* ret)
    {
      return ret;
    }
  private:
    std::unordered_map<Location, T*> reps_;
  }
}

#endif
