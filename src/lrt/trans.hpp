#ifndef EBBRT_LRT_TRANS_HPP
#define EBBRT_LRT_TRANS_HPP

#include <cstddef>

#include "arch/args.hpp"
//#include "lrt/event.hpp"
#include "lrt/arch/trans.hpp"

namespace ebbrt {
  namespace lrt {
    namespace trans {
      bool init(unsigned num_cores);
      void init_ebbs();
      void init_cpu();

      class FuncRet {
      public:
        union {
          void (*func)();
          void* ret;
        };
      };

      typedef uint32_t EbbId;

      class EbbRoot {
      public:
        virtual bool PreCall(Args* args, ptrdiff_t fnum,
                             FuncRet* fret, EbbId id) = 0;
        virtual void* PostCall(void* ret) = 0;
        virtual ~EbbRoot() {}
      };

      class EbbRep {
      };

      class LocalEntry {
      public:
        EbbRep* ref;
        void (*(*rep)[256])(); //pointer to a size 256 array of
                               //pointers to functions taking no
                               //arguments and having no return
      };

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
