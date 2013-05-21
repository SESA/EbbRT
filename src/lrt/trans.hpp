#ifndef EBBRT_LRT_TRANS_HPP
#define EBBRT_LRT_TRANS_HPP

#ifdef LRT_BARE
#include <src/lrt/bare/trans.hpp>
#elif LRT_ULNX
#include <src/lrt/ulnx/trans.hpp>
#endif

#include <cstddef>
#include "arch/args.hpp"

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
       * @brief Per-core initialization
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
       * @brief Ebb Representative
       */
      class EbbRep {
      };

      /**
       * @brief Local translation table entry.  By default the ref of an
       * "empty" entry is pointed to the `shadow` virtual function table
       * which, in turn, will act as the objects virtual function table and
       * call into the corresponding default function that initiates a  miss.
       *
       * For configured entries, the ref is referenced (method is resolved using the objects own
       * v-table), and the `shadow` table is unused.
       */
      class LocalEntry {
      public:
        /* Ebb ref */
        EbbRep* ref;
        /* `shadow` virtual function table*/
        void (*(*rep)[256])(); //pointer to a size 256 array of
                               //pointers to functions taking no
                               //arguments and having no return
      };

      /**
       * @brief Elastic Building Block template
       */
      template <class T>
      class Ebb {
      public:
        /**
         * @brief Ebb Object Constructor
         *
         * @param id Given ebb ID
         */
        explicit Ebb(EbbId id) :
          /* construct the ref corresponding to the given id */
          ref_{ reinterpret_cast<T**> (&(reinterpret_cast<LocalEntry*> (LOCAL_MEM_VIRT)[id]
            ))} {}
        /**
         * @brief Overload arrow operator
         *
         * @return return pointer to ebb reference 
         */
        T* operator->() const {
          return *ref_; 
        }
        /**
         * @brief Get ebb's id
         *
         * @return Ebb id
         */
        operator EbbId() const {
          return reinterpret_cast<LocalEntry*>(ref_) -
            reinterpret_cast<LocalEntry*>(LOCAL_MEM_VIRT);
        }
      private:
        /**
         * @brief And in the end, the ebb you take is equal to the ebb you make.
         */
        T** ref_; 
      };
    }
  }
}


#endif
