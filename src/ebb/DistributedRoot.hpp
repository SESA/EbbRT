#ifndef EBBRT_EBB_DISTRIBUTEDROOT_HPP
#define EBBRT_EBB_DISTRIBUTEDROOT_HPP

#include <map>

#include "ebb/ebb.hpp"
#include "ebb/EbbAllocator/EbbAllocator.hpp"

namespace ebbrt {
  template <typename T>
  class DistributedRoot : public EbbRoot {
  public:
    bool PreCall(Args* args, ptrdiff_t fnum,
                 lrt::trans::FuncRet* fret, EbbId id) override
    {
      auto it = reps_.find(get_location());
      T* ref;
      if (it == reps_.end()) {
        // No rep for this location
        ref = new T();
        ebb_allocator->CacheRep(id, ref);
        reps_[get_location()] = ref;
      } else {
        ebb_allocator->CacheRep(id, it->second);
        ref = it->second;
      }
      *reinterpret_cast<EbbRep**>(args) = ref;
      // rep is a pointer to pointer to array 256 of pointer to
      // function returning void
      void (*(**rep)[256])() = reinterpret_cast<void (*(**)[256])()>(ref);
      fret->func = (**rep)[fnum];
      return true;
    }
    void* PostCall(void* ret) override
    {
      return ret;
    }
  private:
    std::map<Location, T*> reps_;
  };
}

#endif
