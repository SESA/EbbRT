//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_BUFFER_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_BUFFER_H_

#include <cstdlib>
#include <memory>
#include <new>
#include <utility>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#include <boost/container/slist.hpp>
#pragma GCC diagnostic pop

#include <ebbrt/Debug.h>

namespace ebbrt {
struct FreeDeleter {
  void operator()(void* p) const { std::free(p); }
};

class MutableBuffer {
 public:
  explicit MutableBuffer(std::size_t size) : size_(size), offset_(0) {
    auto mem = std::malloc(size);
    if (mem == nullptr)
      throw std::bad_alloc();
    ptr_ = mem;
    free_ = [](void* ptr) { free(ptr); };  // NOLINT
  }

  MutableBuffer(void* ptr, std::size_t size,
                std::function<void(void*)> free_func = [](void* ptr) {
    free(ptr);
  })
      : ptr_(ptr), free_(std::move(free_func)), size_(size), offset_(0) {}

  MutableBuffer(const MutableBuffer&) = delete;
  MutableBuffer& operator=(const MutableBuffer&) = delete;

  MutableBuffer(MutableBuffer&&) = default;
  MutableBuffer& operator=(MutableBuffer&&) = default;

  ~MutableBuffer() {
    if (!free_)
      return;

    kassert(ptr_ != nullptr);
    auto p = static_cast<void*>(static_cast<char*>(ptr_) - offset_);
    free_(p);
  }

  MutableBuffer& operator+=(size_t sz) {
    ptr_ = static_cast<void*>(static_cast<char*>(ptr_) + sz);
    size_ -= sz;
    offset_ += sz;
    return *this;
  }

  void* addr() const { return ptr_; }
  size_t size() const { return size_; }

  std::pair<void*, size_t> release() {
    kassert(ptr_ != nullptr);
    auto p = static_cast<void*>(static_cast<char*>(ptr_) - offset_);
    ptr_ = nullptr;
    free_ = nullptr;
    return std::make_pair(p, size_ + offset_);
  }

 private:
  void* ptr_;
  std::function<void(void*)> free_;
  size_t size_;
  size_t offset_;
};

typedef boost::container::slist<MutableBuffer> MutableBufferList;  // NOLINT

struct NoDeleter {
  void operator()(const void* p) const {}
};

class ConstBuffer {
 public:
  template <typename Deleter = NoDeleter>
  ConstBuffer(const void* ptr, std::size_t size, Deleter&& d = Deleter())
      : size_(size), offset_(0) {
    ptr_ = std::shared_ptr<const void>(ptr, std::forward<Deleter>(d));
  }

  ConstBuffer(const ConstBuffer&) = default;
  ConstBuffer& operator=(const ConstBuffer&) = default;

  ConstBuffer(ConstBuffer&&) = default;
  ConstBuffer& operator=(ConstBuffer&&) = default;

  const void* addr() const {
    auto p = ptr_.get();
    return static_cast<const void*>(static_cast<const char*>(p) + offset_);
  }

  size_t size() const { return size_; }

 private:
  std::shared_ptr<const void> ptr_;
  size_t size_;
  size_t offset_;
};

typedef boost::container::slist<ConstBuffer> ConstBufferList;  // NOLINT
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_BUFFER_H_
