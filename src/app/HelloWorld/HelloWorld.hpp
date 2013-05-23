#ifndef EBBRT_APP_HELLOWORLD_HELLOWORLD_HPP
#define EBBRT_APP_HELLOWORLD_HELLOWORLD_HPP

#include "app/app.hpp"
#include "app/AppEbb.hpp"
#include "sync/spinlock.hpp"

namespace ebbrt {
  class HelloWorldApp : public App {
  public:
    void Start() override;
  private:
    Spinlock lock_;
  };
}

#endif
