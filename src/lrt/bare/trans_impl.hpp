#ifndef EBBRT_LRT_BARE_TRANS_IMPL_HPP
#define EBBRT_LRT_BARE_TRANS_IMPL_HPP

#include <cstddef>
#include <unordered_map>

#include "lrt/trans.hpp"
#include "lrt/bare/arch/trans_impl.hpp"

namespace ebbrt {
  namespace lrt {
    namespace trans {
      extern void (*default_vtable[256])();
      extern LocalEntry** phys_local_entries;
      const std::ptrdiff_t NUM_LOCAL_ENTRIES = LTABLE_SIZE / sizeof(LocalEntry);

      /**
       * @brief Set up and process a miss on the local translation table. 
       * This function is upcalled from assembly, while we are running on an
       * alternative stack.
       *
       * @param args
       * @param fnum
       * @param fret
       *
       * @return 
       */
      extern "C" bool _trans_precall(Args* args,
                                     ptrdiff_t fnum,
                                     FuncRet* fret);

      /**
       * @brief Local miss handle post call
       *
       * @param ret
       *
       * @return 
       */
      extern "C" void* _trans_postcall(void* ret);

      /**
       * @brief Cache a local rep in the translation table
       *
       * @param id Ebb id
       * @param rep rep location
       */
      void cache_rep(EbbId id, EbbRep* rep);

      /** @brief This is the root to be called on all misses to find the
       * appropriate root 
       *
       * @param root
       */
      void install_miss_handler(EbbRoot* root);

      /**
       * @brief Initial root miss handler
       */
      class InitRoot : public EbbRoot {
        bool PreCall(Args* args, ptrdiff_t fnum,
                     FuncRet* fret, EbbId id) override;
        void* PostCall(void* ret) override;
      };
      extern InitRoot init_root;
    }
  }
}

#endif
