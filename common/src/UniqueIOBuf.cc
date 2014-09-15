//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Compiler.h>
#include <ebbrt/UniqueIOBuf.h>

ebbrt::UniqueIOBufOwner::UniqueIOBufOwner(uint8_t* p, size_t capacity)
    : ptr_(p), capacity_(capacity) {}

const uint8_t* ebbrt::UniqueIOBufOwner::Buffer() const { return ptr_; }

size_t ebbrt::UniqueIOBufOwner::Capacity() const { return capacity_; }

void ebbrt::UniqueIOBufOwner::Deleter::operator()(uint8_t* p) {
  if (p != nullptr) {
    auto ptr = p - sizeof(MutUniqueIOBuf);
    free(ptr);
  }
}

std::unique_ptr<ebbrt::MutUniqueIOBuf>
ebbrt::MakeUniqueIOBuf(size_t capacity, bool zero_memory) {
  auto size = sizeof(MutUniqueIOBuf) + capacity;
  void* b;
  if (zero_memory) {
    b = std::calloc(size, 1);
  } else {
    b = std::malloc(size);
  }

  if (unlikely(b == nullptr))
    throw std::bad_alloc();

  auto buf = static_cast<uint8_t*>(b) + sizeof(MutUniqueIOBuf);

  return std::unique_ptr<MutUniqueIOBuf>(new (b) MutUniqueIOBuf(buf, capacity));
}
