#ifndef EBBRT_EBB_SHAREDROOT_HPP
#define EBBRT_EBB_SHAREDROOT_HPP

#include "ebb/ebb.hpp"
#include "ebb/EbbAllocator/EbbAllocator.hpp"

namespace ebbrt {
  template <typename T>
    /**
     * @brief Shared (single-rep) Ebb Root 
     */
  class SharedRoot : public EbbRoot {
  public:
    bool PreCall(Args* args, ptrdiff_t fnum,
                 lrt::trans::FuncRet* fret, EbbId id) override
    {
      ebb_allocator->CacheRep(id, &rep_);
      *reinterpret_cast<EbbRep**>(args) = &rep_;
      // rep is a pointer to pointer to array 256 of pointer to
      // function returning void
      void (*(**rep)[256])() = reinterpret_cast<void (*(**)[256])()>(&rep_);
      fret->func = (**rep)[fnum];
      return true;
    }
    void* PostCall(void* ret) override
    {
      return ret;
    }
  private:
    T rep_;
  };
}

#endif
