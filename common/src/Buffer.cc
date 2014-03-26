//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Buffer.h>

#include <cstdlib>

ebbrt::IOBuf::IOBuf(size_t capacity, bool zero_memory)
    : next_(this), prev_(this), data_(nullptr), buf_(nullptr),
      length_(capacity), capacity_(capacity) {
  void* b;
  if (zero_memory) {
    b = std::calloc(capacity, 1);
  } else {
    b = std::malloc(capacity);
  }

  if (b == nullptr)
    throw std::bad_alloc();

  buf_ = std::shared_ptr<uint8_t>(static_cast<uint8_t*>(b),
                                  [](uint8_t* p) { free(p); });
  data_ = static_cast<uint8_t*>(b);
}

ebbrt::IOBuf::IOBuf(void* buf, size_t capacity,
                    std::function<void(void*)> free_func)
    : next_(this), prev_(this), data_(static_cast<uint8_t*>(buf)),
      buf_(static_cast<uint8_t*>(buf), std::move(free_func)), length_(capacity),
      capacity_(capacity) {}

std::unique_ptr<ebbrt::IOBuf> ebbrt::IOBuf::Create(size_t capacity,
                                                   bool zero_memory) {
  return std::unique_ptr<IOBuf>(new IOBuf(capacity, zero_memory));
}

std::unique_ptr<ebbrt::IOBuf> ebbrt::IOBuf::Clone() const {
  auto buff = CloneOne();
  for (IOBuf* current = next_; current != this; current = current->next_) {
    buff->PrependChain(current->CloneOne());
  }
  return buff;
}

<<<<<<< HEAD
std::unique_ptr<ebbrt::IOBuf> ebbrt::IOBuf::CloneOne() const {
  auto p = new IOBuf(*this);
  p->next_ = p;
  p->prev_ = p;
=======
std::unique_ptr<ebbrt::IOBuf> ebbrt::IOBuf::CloneOne() const{
  auto p = new IOBuf(*this);
  p->next_=p;
  p->prev_=p;
>>>>>>> a63b3e2... IOBuff chaining copy constructor
  return std::unique_ptr<IOBuf>(p);
}

std::unique_ptr<ebbrt::IOBuf>
ebbrt::IOBuf::TakeOwnership(void* buf, size_t capacity,
                            std::function<void(void*)> free_func) {
  return std::unique_ptr<IOBuf>(new IOBuf(buf, capacity, std::move(free_func)));
}

std::unique_ptr<ebbrt::IOBuf> ebbrt::IOBuf::WrapBuffer(const void* buf,
                                                       size_t capacity) {
  // FIXME(dschatz): We should actually prevent the data from being written to
  return TakeOwnership(const_cast<void*>(buf), capacity);
}

std::string ebbrt::IOBuf::ToString() {
  std::string ret;
  for (auto current = next_; current != this; current = current->next_) {
    ret.insert(ret.end(), current->Data(), current->Tail());
  }
  return ret;
}

ebbrt::IOBuf::~IOBuf() {
  while (next_ != this) {
    // Because we don't capture the unique_ptr, the deleter should be called
    next_->Unlink();
  }
}

void ebbrt::IOBuf::PrependChain(std::unique_ptr<IOBuf>&& iobuf) {
  // Take ownership of the chain
  auto other = iobuf.release();

  // Remember the tail of the other chain
  auto other_tail = other->prev_;

  // Attach other
  prev_->next_ = other;
  other->prev_ = prev_;

  // Attach ourselves to the end of the other chain
  other_tail->next_ = this;
  prev_ = other_tail;
}

size_t ebbrt::IOBuf::CountChainElements() const {
  size_t elements = 1;
  for (auto current = next_; current != this; current = current->next_) {
    ++elements;
  }
  return elements;
}

size_t ebbrt::IOBuf::ComputeChainDataLength() const {
  auto len = length_;
  for (auto current = next_; current != this; current = current->next_) {
    len += current->length_;
  }
  return len;
}

ebbrt::IOBuf::ConstIterator ebbrt::IOBuf::cbegin() const {
  return ConstIterator(this, this);
}

ebbrt::IOBuf::ConstIterator ebbrt::IOBuf::cend() const {
  return ConstIterator(nullptr, nullptr);
}

ebbrt::IOBuf::Iterator ebbrt::IOBuf::begin() { return Iterator(this, this); }

ebbrt::IOBuf::Iterator ebbrt::IOBuf::end() {
  return Iterator(nullptr, nullptr);
}
