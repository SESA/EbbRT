//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef HOSTED_SRC_INCLUDE_EBBRT_CONTEXT_H_
#define HOSTED_SRC_INCLUDE_EBBRT_CONTEXT_H_

#include <cstring>
#include <unordered_map>

#include <boost/asio.hpp>

#include <ebbrt/EbbId.h>
#include <ebbrt/LocalEntry.h>

namespace ebbrt {
class Runtime;
class Context {
  Runtime& runtime_;
  size_t index_;
  std::unordered_map<EbbId, LocalEntry> local_table_;

  friend class Runtime;

 public:
  explicit Context(Runtime& runtime);
  Context(Context&&) = default;
  Context& operator=(Context&&) = default;
  void Activate();
  void Deactivate();
  void Run();
  void RunOne();
  void PollOne();
  LocalEntry GetLocalEntry(EbbId id) { return local_table_[id]; }
  void SetLocalEntry(EbbId id, LocalEntry le) { local_table_[id] = le; }

  boost::asio::io_service io_service_;
};

extern thread_local Context* active_context;
}  // namespace ebbrt

#endif  // HOSTED_SRC_INCLUDE_EBBRT_CONTEXT_H_
