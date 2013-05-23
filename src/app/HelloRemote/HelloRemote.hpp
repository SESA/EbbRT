#ifndef EBBRT_APP_HELLOREMOTE_HELLOREMOTE_HPP
#define EBBRT_APP_HELLOREMOTE_HELLOREMOTE_HPP

#include "app/app.hpp"
#include "app/AppEbb.hpp"
#include "sync/spinlock.hpp"

namespace ebbrt {
  class HelloRemoteApp : public App {
  public:
    void Start() override;
  };
}

#endif
