#ifndef EBBRT_LRT_ULNX_TRANS_HPP
#define EBBRT_LRT_ULNX_TRANS_HPP

#include <cstdlib>
namespace ebbrt {
  namespace lrt {
    namespace trans {
      const size_t LTABLE_SIZE = 1 << 21;
      class LocalEntry;
      extern LocalEntry* local_table;
    }
  }
}

#endif
