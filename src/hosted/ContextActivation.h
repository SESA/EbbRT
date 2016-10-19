//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_CONTEXTACTIVATION_H_
#define HOSTED_SRC_INCLUDE_EBBRT_CONTEXTACTIVATION_H_

namespace ebbrt {
class Context;
class ContextActivation {
  Context& c_;

 public:
  explicit ContextActivation(Context& c);
  ~ContextActivation();
};
}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_CONTEXTACTIVATION_H_
