#pragma once

namespace foo {
  class Test {
    int x;
  public:
    Test(int);
    virtual int foo();
    virtual ~Test() {};
  };
}
