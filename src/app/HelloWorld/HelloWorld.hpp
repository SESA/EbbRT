#ifndef EBBRT_APP_HELLOWORLD_HELLOWORLD_HPP
#define EBBRT_APP_HELLOWORLD_HELLOWORLD_HPP

#include <atomic>

#include "app/app.hpp"
#include "app/AppEbb.hpp"

namespace ebbrt {
  class HelloWorldApp : public App {
  public:
    HelloWorldApp();
    void Start() override;
  };
}

#endif
