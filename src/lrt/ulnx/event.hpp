#ifndef EBBRT_LRT_EVENT_HPP
#error "Don't include this file directly"
#endif


namespace {
  /* kludged out */
  int num_cores = 1;
}

namespace ebbrt {
  namespace lrt {
    namespace event {
      extern uintptr_t** altstack;
      /**
       * @brief Get core/thread count
       *
       * @return 
       */
      inline unsigned
      get_num_cores()
      {
        return num_cores;
      }

      /**
       * @brief Get cores location
       *
       * @return 
       */
      inline Location
      get_location()
      {
        /* kludged out */
        return 1;
      }
      bool init_arch(); 
      void init_cpu_arch()__attribute__((noreturn));
    }
  }
}
