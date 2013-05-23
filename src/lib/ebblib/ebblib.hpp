#include "app/app.hpp"
#include "app/AppEbb.hpp"

namespace ebblib {
  void init();
}

namespace ebbrt {
  class FrontEndApp : public App {
  public:
    FrontEndApp();
    void Start() override;
  };
}
