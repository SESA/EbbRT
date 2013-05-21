#ifndef EBBRT_APP_APP_HPP
#define EBBRT_APP_APP_HPP

#ifdef LRT_BARE
#include <src/app/bare/app.hpp>
#elif LRT_ULNX
#include <src/app/ulnx/app.hpp>
#endif

#include "lrt/trans.hpp"

namespace ebbrt {
  namespace app {
    /**
     * @brief Entrance into ebb-world
     */
    void start();
  }
}

#endif
