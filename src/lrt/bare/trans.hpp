#ifndef EBBRT_LRT_BARE_TRANS_HPP
#define EBBRT_LRT_BARE_TRANS_HPP

#include <cstddef>

#include "arch/args.hpp"
//#include "lrt/event.hpp"
#include "lrt/bare/arch/trans.hpp"

namespace ebbrt {
  namespace lrt {
    namespace trans {
      /**
       * @brief Initial translation setup
       *
       * @param num_cores
       *
       * @return 
       */
      bool init(unsigned num_cores);
      /**
       * @brief Construct roots for statically linked Ebbs
       */
      void init_ebbs();
      /**
       * @brief Per-core initilization 
       */
      void init_cpu();
      /**
       * @brief Translation lookup return type
       */
      class FuncRet {
      public:
        union {
          void (*func)();
          void* ret;
        };
      };
      typedef uint32_t EbbId;

      /**
       * @brief Ebb Factory
       */
      class EbbRoot {
      public:
        /**
         * @brief Initial stage of ebb call
         *
         * @param args Ebbcall arguments
         * @param fnum 
         * @param fret 
         * @param id
         *
         * @return 
         */
        virtual bool PreCall(Args* args, ptrdiff_t fnum,
                             FuncRet* fret, EbbId id) = 0;
        /**
         * @brief Final stage of ebb call 
         s
         * @param ret
         *
         * @return 
         */
        virtual void* PostCall(void* ret) = 0;

        virtual ~EbbRoot() {}
      };

      /**
       * @brief 
       */
      class EbbRep {
      };

      /**
       * @brief Local translation table entry
       */
      class LocalEntry {
      public:
        EbbRep* ref;
        void (*(*rep)[256])(); //pointer to a size 256 array of
                               //pointers to functions taking no
                               //arguments and having no return
      };

      /**
       * @brief Elasic Building Block template 
       */
      template <class T>
      class Ebb {
      public:
        explicit Ebb(EbbId id) : id_(id) {}
        T* operator->() const {
          return reinterpret_cast<T*>
            (reinterpret_cast<LocalEntry*>(LOCAL_MEM_VIRT)[id_].ref);
        }
        operator EbbId() const {
          return id_;
        }
      private:
        EbbId id_;
      };
    }
  }
}

#endif
