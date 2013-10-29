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
  return ebbrt::async([](){return 84;});
}

void
setup()
{
  std::cout << "Setup" << std::endl;

  // Call async to execute a function asynchronously on its own event
  ebbrt::async([]() {
      std::cout << "Hello Asynchronous World" << std::endl;
    });

  // Async returns a Future which can be used to get the return value
  // of the function
  ebbrt::Future<int> future = ebbrt::async([]() {
      std::cout << "Hello Asynchronous Futures" << std::endl;
      return 42;
    });

  // Call Then to get the return value out of the future
  future.Then([](ebbrt::Future<int> fval) {
      std::cout << "Got " << fval.Get() << " from a Future" << std::endl;
    });

  // Futures can be returned and passed (copying is not allowed)
  ebbrt::Future<int> future2 = MyAsyncApi();
  // Call Then to get the return value out of the future
  future2.Then([](ebbrt::Future<int> fval) {
      std::cout << "Got " << fval.Get() <<
        " from a Future returned from another function" << std::endl;
    });

  // If a future can be fulfilled synchronously, then just make it
  ebbrt::Future<int> future3 = ebbrt::make_ready_future<int>(168);

  // A user can check if a future is Ready()
  if (future3.Ready()) {
    // And Get() the value synchronously
    std::cout << "Got " << future3.Get() << " synchronously from a Future"
              << std::endl;
  }

  ebbrt::Future<int> future4 = ebbrt::async([](){return 22;});

  // Careful! calling Get() on a ebbrt::Future that isn't Ready() will throw
  // an exception
  try {
    future4.Get();
  } catch (std::exception& e) {
    std::cerr << "Caught exception: " << e.what() << std::endl;
  }

  ebbrt::Future<int> future5 = ebbrt::make_ready_future<int>(33);

  // Then() will call the function synchronously if the future is
  // Ready()
  future5.Then([](ebbrt::Future<int> fval) {
      std::cout << "Got " << fval.Get() <<
        " synchronously from a Future using Then()" << std::endl;
    });

  ebbrt::Future<int> future6 = ebbrt::make_ready_future<int>(44);

  // To prevent the function from being called synchronously, pass
  // in a different launch policy
  future6.Then(ebbrt::launch::async, [](ebbrt::Future<int> fval) {
      std::cout << "Got " << fval.Get() <<
        " asynchronously from a Future using Then()" << std::endl;
    });

  // Futures can also capture exceptions
  ebbrt::Future<void> future7 =
    ebbrt::async([](){
        throw std::runtime_error("Exception thrown asynchronously");
      });

  future7.Then([](ebbrt::Future<void> fval) {
      //Once the future is Ready(), Get() will rethrow the exception
      try {
        fval.Get();
      } catch (std::exception& e) {
        std::cout << "Caught exception from Future: " << e.what() << std::endl;
      }
    });

  ebbrt::Future<void> future8 = ebbrt::async([](){});

  // Then() actually returns a future containing the result of the
  // function. This allows chaining asynchronous calls onto each
  // other.
  future8.Then([](ebbrt::Future<void> fval) {
      std::cout << "Printed Asynchronously x1" << std::endl;
  }).Then([](ebbrt::Future<void> fval) {
      std::cout << "Printed Asynchronously x2" << std::endl;
  }).Then([](ebbrt::Future<void> fval) {
      std::cout << "Printed Asynchronously x3" << std::endl;
  });

  // Futures can be default constructed
  ebbrt::Future<int> future9;
  if (!future9.Valid()) {
    std::cout << "not a valid future" << std::endl;
  }

  // Nested futures make little sense, but sometimes they arise
  ebbrt::Future<ebbrt::Future<int> > future10 =
    ebbrt::make_ready_future<ebbrt::Future<int> >(ebbrt::make_ready_future<int>(81));

  // flatten() will flatten a nested future until it is no longer nested
  ebbrt::Future<int> future11 = flatten(std::move(future10));

  future11.Then([](ebbrt::Future<int> fval) {
      std::cout << "Got val: " << fval.Get() <<
        " from flattened future" << std::endl;
    });


  // async() will automatically flatten the result
  ebbrt::Future<int> future12 = ebbrt::async([](){
      return ebbrt::async([](){
          return 16;
        });
    });

  future12.Then([](ebbrt::Future<int> fval) {
      std::cout << "Got val: " << fval.Get() <<
        " from flattened future from async" << std::endl;
    });

  ebbrt::Future<void> future13 = ebbrt::make_ready_future<void>();

  // Then() also flattens the result
  ebbrt::Future<int> future14 = future13.Then([](ebbrt::Future<void> fval) {
      return ebbrt::async([](){
          return 72;
        });
    });

  future14.Then([](ebbrt::Future<int> fval) {
      std::cout << "Got val: " << fval.Get() <<
        " from flattened future from Then()" << std::endl;
    });

  std::cout << "Setup complete" << std::endl;
}

#ifdef __ebbrt__
void
ebbrt::app::start()
{
  setup();
}
#endif

#ifdef __linux__
main(int argc, char* argv[] )
{
  if(argc < 2)
  {
    std::cout << "Usage: dtb as first argument\n";
    std::exit(1);
  }

  int n;
  char *fdt = ebbrt::app::LoadFile(argv[1], &n);

  // ...fdt buffer contains the entire file...
  ebbrt::EbbRT instance((void *)fdt);
  ebbrt::Context context{instance};
  context.Activate();
  setup();
  context.Loop(-1);
  return 0;
}
#endif
