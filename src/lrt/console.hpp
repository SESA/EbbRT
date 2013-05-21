#ifdef LRT_ULNX
#include <src/lrt/ulnx/console.hpp>
#endif
/* baremetal console implemented in uart */

namespace ebbrt {
  namespace lrt {
    namespace console {
      void init();
      void write(char c);
      int write(const char *str, int len);
      void write(const char *str);
    }
  }
}
