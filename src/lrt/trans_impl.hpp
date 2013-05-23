#ifndef EBBRT_LRT_TRANS_IMPL_HPP
#define EBBRT_LRT_TRANS_IMPL_HPP

#ifdef LRT_ULNX
#include <src/lrt/ulnx/trans_impl.hpp>
#elif LRT_BARE
#include <src/lrt/bare/trans_impl.hpp>
#endif

#include <cstddef>
#include <unordered_map>

#include "lrt/trans.hpp"

namespace ebbrt {
  namespace lrt {
    namespace trans {
      /** virtual function table */
      extern void (*default_vtable[256])();
      /**  physical addresses of local translation tables */
      extern LocalEntry** phys_local_entries;
      const std::ptrdiff_t NUM_LOCAL_ENTRIES = LTABLE_SIZE / sizeof(LocalEntry);

      /**
       * @brief Set up and process a miss on the local translation system.
       * This function is upcalled from assembly, while running on the
       * alternative stack.
       *
       * @param args Arguments of the `missed` ebb call
       * @param fnum Function number of call
       * @param fret Pointer to return value of call
       *
       * @return
       */
      extern "C" bool _trans_precall(Args* args,
                                     ptrdiff_t fnum,
                                     FuncRet* fret);

      /**
       * @brief Local miss handle post call. Upcalled from assembly
       *
       * @param ret
       *
       * @return
       */
      extern "C" void* _trans_postcall(void* ret);

      /**
       * @brief Cache ebb rep in the local translation system. This is called
       * most likely called from the ebb manager.
       *
       * @param id Ebb's id
       * @param rep Location of location ebb representative
       */
      void cache_rep(EbbId id, EbbRep* rep);

      /**
       * @brief Swap out existing miss handler
       *
       * @param root
       */
      void install_miss_handler(EbbRoot* root);

      class RootBinding {
      public:
        ebbrt::lrt::trans::EbbId id;
        ebbrt::lrt::trans::EbbRoot* root;
      };

      extern RootBinding* initial_root_table;

      /**
       * @brief InitRoot acts as our initial root miss handler / translation
       * ebb that works off the initial root table constructed on bring up.
       * InitRoot will be replaced by the event manager upon the miss on an
       * ebb not in the static root table.
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
