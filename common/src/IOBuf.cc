//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/IOBuf.h>

#include <cstdlib>

ebbrt::IOBuf::IOBuf(const uint8_t* data, size_t length) noexcept
    : data_(data),
      length_(length) {}

ebbrt::IOBuf::~IOBuf() noexcept {
  while (Next() != this) {
    // Because we don't capture the unique_ptr, the deleter should be called
    Next()->Unlink();
  }
}

void ebbrt::IOBuf::AdvanceChain(size_t amount) {
  assert(ComputeChainDataLength() >= amount);
  for (auto& buf : *this) {
    auto advance_len = std::min(buf.Length(), amount);
    buf.Advance(advance_len);
    amount -= advance_len;
    if (amount == 0)
      break;
  }
}

void ebbrt::IOBuf::PrependChain(std::unique_ptr<IOBuf> iobuf) {
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

ebbrt::MutIOBuf::MutIOBuf(const uint8_t* data, size_t length) noexcept
    : IOBuf(data, length) {}

ebbrt::MutIOBuf::Iterator ebbrt::MutIOBuf::begin() {
  return Iterator(this, this);
}

ebbrt::MutIOBuf::Iterator ebbrt::MutIOBuf::end() {
  return Iterator(nullptr, nullptr);
}
