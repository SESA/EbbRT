#ifndef EBBRT_LRT_TRANS_IMPL_HPP
#define EBBRT_LRT_TRANS_IMPL_HPP

#include <cstddef>
#include <unordered_map>

#include "lrt/trans.hpp"
#include "lrt/arch/trans_impl.hpp"

namespace ebbrt {
  namespace lrt {
    namespace trans {
      extern void (*default_vtable[256])();
      extern LocalEntry** phys_local_entries;
      const std::ptrdiff_t NUM_LOCAL_ENTRIES = LTABLE_SIZE / sizeof(LocalEntry);

      extern "C" bool _trans_precall(Args* args,
                                     ptrdiff_t fnum,
                                     FuncRet* fret);

      extern "C" void* _trans_postcall(void* ret);

      void cache_rep(EbbId id, EbbRep* rep);
      // This is the root to be called on all misses to find the appropriate
      // root
      void install_miss_handler(EbbRoot* root);

      class InitRoot : public EbbRoot {
        bool PreCall(Args* args, ptrdiff_t fnum, FuncRet* fret) override;
        void* PostCall(void* ret) override;
      };
      extern InitRoot init_root;
    }
  }
}

#endif
