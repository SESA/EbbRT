//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef COMMON_SRC_INCLUDE_EBBRT_BUFFER_H_
#define COMMON_SRC_INCLUDE_EBBRT_BUFFER_H_

#include <cstring>
#include <memory>

#include <boost/iterator/iterator_facade.hpp>

namespace ebbrt {
namespace detail {
// Is T a unique_ptr<> to a standard-layout type
template <class T, class Enable = void>
struct IsUniquePtrToSL : public std::false_type {};

template <class T, class D>
struct IsUniquePtrToSL<std::unique_ptr<T, D>,
                       typename std::enable_if<std::is_standard_layout<
                           T>::value>::type> : public std::true_type {};
}  // namespace detail
class IOBuf {
 public:
  class ConstIterator;
  class Iterator;

  IOBuf() noexcept;
  explicit IOBuf(size_t capacity, bool zero_memory = false);
  IOBuf(void* buf, size_t capacity, std::function<void(void*)> free_func);
  IOBuf(IOBuf&& other) noexcept;
  IOBuf& operator=(IOBuf&& other) noexcept;

  static std::unique_ptr<IOBuf> Create(size_t capacity,
                                       bool zero_memory = false);

  std::unique_ptr<IOBuf> Clone() const;
  std::unique_ptr<IOBuf> CloneOne() const;

  static std::unique_ptr<IOBuf>
  TakeOwnership(void* buf, size_t capacity,
                std::function<void(void*)> free_func = [](void*) {});

  template <class UniquePtr>
  static typename std::enable_if<detail::IsUniquePtrToSL<UniquePtr>::value,
                                 std::unique_ptr<IOBuf>>::type
  TakeOwnership(UniquePtr&& buf, size_t count = 1);

  static std::unique_ptr<IOBuf> WrapBuffer(const void* buf, size_t size);

  static std::unique_ptr<IOBuf> CopyBuffer(const void* buf, size_t size) {
    auto b = Create(size);
    std::memcpy(b->WritableData(), buf, size);
    return b;
  }

  static std::unique_ptr<IOBuf> CopyBuffer(const std::string& buf) {
    return CopyBuffer(buf.data(), buf.size());
  }

  static std::unique_ptr<IOBuf> Clone(const std::unique_ptr<const IOBuf>& buf);

  static void Destroy(std::unique_ptr<IOBuf>&& data) {
    auto destroyer = std::move(data);
  }

  std::string ToString();

  ~IOBuf();

  bool Empty() const;

  const uint8_t* Data() const { return data_; }

  uint8_t* WritableData() { return data_; }

  const uint8_t* Tail() const { return data_ + length_; }

  uint8_t* WritableTail() { return data_ + length_; }

  size_t Length() const { return length_; }

  const uint8_t* Buffer() const { return buf_.get(); }

  uint8_t* WritableBuffer() { return buf_.get(); }

  const uint8_t* BufferEnd() const { return buf_.get() + capacity_; }

  uint64_t Capacity() const { return capacity_; }

  IOBuf* Next() { return next_; }

  const IOBuf* Next() const { return next_; }

  IOBuf* Prev() { return prev_; }

  const IOBuf* Prev() const { return prev_; }

  void Advance(size_t amount) {
    data_ += amount;
    length_ -= amount;
  }

  void Retreat(size_t amount) {
    data_ -= amount;
    length_ += amount;
  }

  void TrimEnd(size_t amount) { length_ -= amount; }

  bool IsChained() const { return next_ != this; }

  size_t CountChainElements() const;

  size_t ComputeChainDataLength() const;

  void PrependChain(std::unique_ptr<IOBuf>&& iobuf);
  void AppendChain(std::unique_ptr<IOBuf>&& iobuf) {
    next_->PrependChain(std::move(iobuf));
  }

  /* Remove this IOBuf from its current chain and return a unique_ptr to it. Do
   * not call on the head of a chain that is owned already */
  std::unique_ptr<IOBuf> Unlink() {
    next_->prev_ = prev_;
    prev_->next_ = next_;
    prev_ = this;
    next_ = this;
    return std::unique_ptr<IOBuf>(this);
  }

  /* Remove this IOBuf from its current chain and return a unique_ptr to the
     IOBuf that formerly followed it in the chain */
  std::unique_ptr<IOBuf> Pop() {
    IOBuf* next = next_;
    next_->prev_ = prev_;
    prev_->next_ = next_;
    prev_ = this;
    next_ = this;
    return std::unique_ptr<IOBuf>((next == this) ? nullptr : next);
  }

  bool Unique() const {
    const IOBuf* current = this;
    while (true) {
      if (current->UniqueOne()) {
        return true;
      }
      current = current->next_;
      if (current == this) {
        return false;
      }
    }
  }

  bool UniqueOne() const { return buf_.unique(); }

  ConstIterator cbegin() const;
  ConstIterator cend() const;

  Iterator begin();
  Iterator end();

  ConstIterator begin() const;
  ConstIterator end() const;

  class DataPointer {
   public:
    explicit DataPointer(const IOBuf* p) : p_(p) {}

    const uint8_t* Data() {
      if (!p_ || p_->Length() - offset_ == 0)
        throw std::runtime_error("DataPointer::Data() past end of buffer");

      return p_->Data() + offset_;
    }

    template <typename T> const T& GetNoAdvance() {
      if (!p_)
        throw std::runtime_error("DataPointer::Get(): past end of buffer");

      if (p_->Length() - offset_ < sizeof(T)) {
        throw std::runtime_error(
            "DataPointer::Get(): request straddles buffers");
      }

      return *reinterpret_cast<const T*>(p_->Data() + offset_);
    }

    template <typename T> const T& Get() {
      auto& ret = GetNoAdvance<T>();
      Advance(sizeof(T));
      return ret;
    }

    void Advance(size_t size) {
      if (!p_)
        throw std::runtime_error("DataPointer::Advance(): past end of buffer");

      auto remainder = p_->Length() - offset_;
      if (remainder > size) {
        offset_ += size;
      } else {
        p_ = p_->Next();
        offset_ = size - remainder;
      }
    }

   private:
    const IOBuf* p_{nullptr};
    size_t offset_{0};
  };

  DataPointer GetDataPointer() const { return DataPointer(this); }

 private:
  IOBuf(const IOBuf&) = default;
  IOBuf& operator=(const IOBuf&) = default;
  IOBuf* next_{this};
  IOBuf* prev_{this};
  uint8_t* data_{nullptr};
  std::shared_ptr<uint8_t> buf_{nullptr};
  size_t length_{0};
  size_t capacity_{0};
};

class IOBuf::Iterator
    : public boost::iterator_facade<IOBuf::Iterator, IOBuf,
                                    boost::forward_traversal_tag> {
 public:
  Iterator(IOBuf* pos, IOBuf* end) : pos_(pos), end_(end) {}

 private:
  IOBuf& dereference() const { return *pos_; }

  bool equal(const Iterator& other) const {
    return pos_ == other.pos_ && end_ == other.end_;
  }

  void increment() {
    pos_ = pos_->Next();

    if (pos_ == end_)
      pos_ = end_ = nullptr;
  }

  IOBuf* pos_;
  IOBuf* end_;

  friend class boost::iterator_core_access;
};

class IOBuf::ConstIterator
    : public boost::iterator_facade<IOBuf::ConstIterator, const IOBuf,
                                    boost::forward_traversal_tag> {
 public:
  ConstIterator(const IOBuf* pos, const IOBuf* end) : pos_(pos), end_(end) {}

 private:
  const IOBuf& dereference() const { return *pos_; }

  bool equal(const ConstIterator& other) const {
    return pos_ == other.pos_ && end_ == other.end_;
  }

  void increment() {
    pos_ = pos_->Next();

    if (pos_ == end_)
      pos_ = end_ = nullptr;
  }

  const IOBuf* pos_;
  const IOBuf* end_;

  friend class boost::iterator_core_access;
};

inline IOBuf::ConstIterator IOBuf::begin() const { return cbegin(); }
inline IOBuf::ConstIterator IOBuf::end() const { return cend(); }
}  // namespace ebbrt

#endif  // COMMON_SRC_INCLUDE_EBBRT_BUFFER_H_
