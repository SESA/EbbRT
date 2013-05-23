#ifndef EBBRT_EBB_SHAREDROOT_HPP
#define EBBRT_EBB_SHAREDROOT_HPP

#include "ebb/ebb.hpp"
#include "ebb/EbbManager/EbbManager.hpp"

namespace ebbrt {
  template <typename T>
    /**
     * @brief Shared Ebb Root
     * A single ebb representaive is shared between all locations
     */
    class SharedRoot : public EbbRoot {
      public:
        /**
         * @brief Handle the construction of the rep. This gets called on a
         * miss to cache the local representative.
         *
         * @param args Ebb call arguments
         * @param fnum Ebb call function number
         * @param fret Return value address
         * @param id   Ebb Id
         *
         * @return
         */
        bool PreCall(Args* args, ptrdiff_t fnum,
            lrt::trans::FuncRet* fret, EbbId id) override
        {
          ebb_manager->CacheRep(id, &rep_);
          *reinterpret_cast<EbbRep**>(args) = &rep_;
          // rep is a pointer to pointer to array 256 of pointer to
          // function returning void
          void (*(**rep)[256])() = reinterpret_cast<void (*(**)[256])()>(&rep_);
          fret->func = (**rep)[fnum];
          return true;
        }
        /**
         * @brief Ebb construction post-call.
         *
         * @param ret ebb calls return
         *
         * @return Return ebb call return value
         */
        void* PostCall(void* ret) override
        {
          return ret;
        }
      private:
        /**
         * @brief Our single shared representative
         */
        T rep_;
    };
}

#endif
