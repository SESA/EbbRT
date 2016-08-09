//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef CMDLINEARGS_H_
#define CMDLINEARGS_H_

#include <ebbrt/EbbAllocator.h>
#include <ebbrt/Future.h>

class CmdLineArgs {
public:
  CmdLineArgs(std::vector<std::string> args);

  static CmdLineArgs &HandleFault(ebbrt::EbbId id);
  static ebbrt::Future<ebbrt::EbbRef<CmdLineArgs> >
  Create(int argc, char **argv,
         ebbrt::EbbId id = ebbrt::ebb_allocator->Allocate());

  int argc() { return args_.size(); }
  char **argv() { return argv_.data(); }

private:
  std::vector<std::string> args_;
  std::vector<char *> argv_;
};

#endif  // CMDLINEARGS_H_
