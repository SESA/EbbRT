#include <iostream>
#include <sstream>

#ifdef __ebbrt__
#include "src/app/app.hpp"
#endif

#include "src/ebb/Console/Console.hpp"
#include "src/ebb/EventManager/Future.hpp"

ebbrt::Future<int>
MyAsyncApi()
{
  return ebbrt::Future<int>(42);
}

ebbrt::Future<int>
MyAsyncApi2()
{
  return ebbrt::async([](){return 42;});
}

void
setup()
{
  ebbrt::console->Write("Setup\n");

  auto fut = MyAsyncApi();
  fut.OnSuccess([](int val) {
      std::ostringstream str;
      str << "Got val asynchronously: " << val << std::endl;
      ebbrt::console->Write(str.str().c_str());
    });

  if (fut.Fulfilled()) {
    auto val = fut.Get();
    std::ostringstream str;
    str << "Got val (if): " << val << std::endl;
    ebbrt::console->Write(str.str().c_str());
  }

  try {
    auto val = fut.Get();
    std::ostringstream str;
    str << "Got val (exception): " << val << std::endl;
    ebbrt::console->Write(str.str().c_str());
  } catch (std::exception& e) {
    std::ostringstream str;
    str << "Got exception: " << e.what() << std::endl;
    ebbrt::console->Write(str.str().c_str());
  }

  ebbrt::async([]() {
      ebbrt::console->Write("Printed asynchronously\n");
    }).OnSuccess([]() {
        ebbrt::console->Write("Printed asynchronously x2\n");
      }).OnSuccess([]() {
          ebbrt::console->Write("Printed asynchronously x3\n");
        });

  auto fut2 = MyAsyncApi2();
  fut2.OnSuccess([](int val) {
      std::ostringstream str;
      str << "Got val asynchronously: " << val << std::endl;
      ebbrt::console->Write(str.str().c_str());
    });

  if (fut2.Fulfilled()) {
    auto val = fut2.Get();
    std::ostringstream str;
    str << "Got val (if): " << val << std::endl;
    ebbrt::console->Write(str.str().c_str());
  } else {
    fut2.OnSuccess([](int val) {
        std::ostringstream str;
        str << "Got val (if) asynchronously: " << val << std::endl;
        ebbrt::console->Write(str.str().c_str());
      });
  }

  try {
    auto val = fut2.Get();
    std::ostringstream str;
    str << "Got val (exception): " << val << std::endl;
    ebbrt::console->Write(str.str().c_str());
  } catch (std::exception& e) {
    std::ostringstream str;
    str << "Got exception: " << e.what() << std::endl;
    ebbrt::console->Write(str.str().c_str());
    fut2.OnSuccess([](int val) {
        std::ostringstream str;
        str << "Got val (exception) asynchronously: " << val << std::endl;
        ebbrt::console->Write(str.str().c_str());
      });
  }

  auto fut3 = ebbrt::async([]() {
      throw std::runtime_error("Runtime exception!");
    });

  fut3.OnFailure([](std::exception_ptr eptr) {
      try {
        std::rethrow_exception(eptr);
      } catch (std::runtime_error& e) {
        std::ostringstream str;
        str << "Future failed asynchronously: " << e.what() << std::endl;
        ebbrt::console->Write(str.str().c_str());
      }
    });

  // async([]() {
  //     return async([]() {
  //         return 81;
  //       });
  //   }).OnSuccess([](int val){
  //       std::cout << "Got val: " << val << " from flattened future" << std::endl;
  //     });

  // async([]() {
  //     return Future<int> {81};
  //   }).OnSuccess([](int val){
  //       std::cout << "Got val: " << val << " from flattened future" << std::endl;
  //     });

  ebbrt::console->Write("Setup complete\n");
}

#ifdef __ebbrt__
void
ebbrt::app::start()
{
  setup();
}
#endif

#ifdef __linux__
int main()
{
  ebbrt::EbbRT instance;

  ebbrt::Context context{instance};
  context.Activate();
  setup();
  context.Loop(-1);
  return 0;
}
#endif
