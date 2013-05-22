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
      inline unsigned
      get_num_cores()
      {
        return num_cores;
      }

      inline Location
      get_location()
      {
        /* kludged out */
        return 1;
      }
    }
  }
}
