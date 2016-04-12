//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/IOBufRef.h>

const constexpr ebbrt::IOBufRefOwner::CloneView_s
    ebbrt::IOBufRefOwner::CloneView;

ebbrt::IOBufRefOwner::IOBufRefOwner(const IOBuf& buf)
    : buffer_(buf.Buffer()), capacity_(buf.Capacity()) {}

const uint8_t* ebbrt::IOBufRefOwner::Buffer() const { return buffer_; }

size_t ebbrt::IOBufRefOwner::Capacity() const { return capacity_; }

std::unique_ptr<ebbrt::IOBufRef> ebbrt::CreateRef(const IOBuf& buf) {
  auto head = std::unique_ptr<ebbrt::IOBufRef>(new IOBufRef(buf));
  head->Advance(buf.Data() - buf.Buffer());
  head->TrimEnd(head->Length() - buf.Length());

  return head;
}

std::unique_ptr<ebbrt::MutIOBufRef> ebbrt::CreateRef(const MutIOBuf& buf) {
  auto p = CreateRef(static_cast<const IOBuf&>(buf));
  auto d = reinterpret_cast<MutIOBufRef*>(p.release());
  return std::unique_ptr<MutIOBufRef>(d);
}

std::unique_ptr<ebbrt::IOBufRef> ebbrt::CreateRefChain(const IOBuf& buf) {
  auto head = CreateRef(buf);

  for (auto it = ++(buf.begin()); it != buf.end(); ++it) {
    head->PrependChain(CreateRef(*it));
  }

  return head;
}

std::unique_ptr<ebbrt::MutIOBufRef> ebbrt::CreateRefChain(const MutIOBuf& buf) {
  auto p = CreateRefChain(static_cast<const IOBuf&>(buf));
  auto d = reinterpret_cast<MutIOBufRef*>(p.release());
  return std::unique_ptr<MutIOBufRef>(d);
}
