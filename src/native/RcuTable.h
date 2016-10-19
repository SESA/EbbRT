//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_RCUTABLE_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_RCUTABLE_H_

#include <cassert>
#include <functional>

#include "../AtomicUniquePtr.h"
#include "RcuList.h"

namespace ebbrt {

template <typename T, RcuHListHook T::*hookptr> class RcuBuckets {
  typedef RcuHList<T, hookptr> bucket_type;

  std::size_t size_;
  bucket_type buckets_[0];

  explicit RcuBuckets(size_t size) : size_(size) {
    // placement new buckets array
    new (static_cast<void*>(&buckets_)) bucket_type[size_];
    for (unsigned i = 0; i < size_; ++i) {
      buckets_[i].clear();
    }
    std::atomic_thread_fence(std::memory_order_release);
  }

  RcuBuckets(const RcuBuckets& buckets, std::size_t sz) {
    new (static_cast<void*>(&buckets_)) bucket_type[size_];
    for (unsigned i = 0; i < sz; ++i) {
      buckets_[i] = buckets.buckets_[i];
    }
    std::atomic_thread_fence(std::memory_order_release);
  }

  ~RcuBuckets() {
    for (unsigned i = 0; i < size_; ++i) {
      buckets_[i].~bucket_type();
    }
  }

 public:
  struct Deleter {
    void operator()(RcuBuckets* ptr) { Destroy(ptr); }
  };

  static RcuBuckets* Create(std::size_t sz) {
    auto ptr = malloc(sizeof(RcuBuckets) + sizeof(bucket_type) * sz);
    if (!ptr)
      throw std::bad_alloc();

    return new (ptr) RcuBuckets(sz);
  }

  static RcuBuckets* Create(const RcuBuckets& buckets, std::size_t sz) {
    auto ptr = malloc(sizeof(RcuBuckets) + sizeof(bucket_type) * sz);
    if (!ptr)
      throw std::bad_alloc();

    return new (ptr) RcuBuckets(buckets, sz);
  }

  static void Destroy(RcuBuckets* ptr) {
    ptr->~RcuBuckets();
    free(ptr);
  }

  std::size_t size() const { return size_; }

  void clear() {
    for (unsigned i = 0; i < size_; ++i) {
      buckets_[i].clear();
    }
    std::atomic_thread_fence(std::memory_order_release);
  }

  bucket_type& get_bucket(std::size_t index) {
    assert(index < size_);
    return buckets_[index];
  }

  const bucket_type& get_bucket(std::size_t index) const {
    assert(index < size_);
    return buckets_[index];
  }

  bucket_type* begin() { return &buckets_[0]; }
  bucket_type* end() { return &buckets_[size_]; }

  const bucket_type* cbegin() const { return &buckets_[0]; }
  const bucket_type* cend() const { return &buckets_[size_]; }
};

template <typename T, typename Key, RcuHListHook T::*hookptr, Key T::*keyptr,
          typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
class RcuHashTable {
  typedef RcuBuckets<T, hookptr> buckets_t;
  atomic_unique_ptr<buckets_t, typename buckets_t::Deleter> buckets_;
  Hash hash_fn_;
  KeyEqual key_equal_fn_;

  buckets_t* get_buckets() { return buckets_.get(); }

  class Unzipper {
    buckets_t* old_buckets_;
    size_t new_sz_;
    Hash hash_fn_;
    size_t hash(const T& node) { return hash_fn_(node.*keyptr); }

   public:
    Unzipper(buckets_t* buckets, size_t new_sz)
        : old_buckets_(buckets), new_sz_(new_sz), hash_fn_(Hash()) {}

    Future<void> unzip() {
      auto moved_one = false;
      for (size_t i = 0; i < old_buckets_->size(); ++i) {
        auto& old_bucket = old_buckets_->get_bucket(i);
        if (old_bucket.empty())
          continue;

        typename RcuHList<T, hookptr>::iterator prev_entry = old_bucket.begin();
        typename RcuHList<T, hookptr>::iterator diff_entry = old_bucket.end();
        // find first entry which hashes to a different new bucket than
        // prev_entry
        for (auto it = std::next(prev_entry); it != old_bucket.end(); ++it) {
          if (hash(*it) != hash(*prev_entry)) {
            diff_entry = it;
            break;
          }
          prev_entry = it;
        }

        // advance old_bucket to the first entry with a different new bucket
        if (diff_entry == old_bucket.end()) {
          old_bucket.set(*diff_entry);
        } else {
          old_bucket.clear();
          continue;  // nothing to chain
        }

        moved_one = true;
        typename RcuHList<T, hookptr>::iterator next_entry = old_bucket.end();
        // first first entry *after* diff_entry that hashes to the same bucket
        // as prev_entry
        for (auto it = std::next(diff_entry); it != old_bucket.end(); ++it) {
          if (hash(*it) == hash(*prev_entry)) {
            next_entry = it;
            break;
          }
        }
        if (next_entry == old_bucket.end()) {
          RcuHList<T, hookptr>::clear_after(prev_entry);
        } else {
          RcuHList<T, hookptr>::splice_after(*next_entry, prev_entry);
        }
      }
      // If we advanced any bucket, we wait for a grace period to elapse and do
      // it again, otherwise just wait for a grace period to elapse and the
      // resize is complete
      if (moved_one) {
        return CallRcu([this]() { return unzip(); });
      } else {
        auto old_buckets = old_buckets_;
        delete this;
        return CallRcu([old_buckets]() { buckets_t::Destroy(old_buckets); });
      }
    }
  };

 public:
  explicit RcuHashTable(uint8_t buckets_shift)
      : buckets_(buckets_t::Create(1 << buckets_shift)), hash_fn_(Hash()),
        key_equal_fn_(KeyEqual()) {}

  T* find(const Key& k) {
    auto b = get_buckets();
    assert(b);

    auto hash = hash_fn_(k);
    auto index = hash % b->size();
    auto& bucket = b->get_bucket(index);
    for (auto& element : bucket) {
      auto& element_key = element.*keyptr;
      if (key_equal_fn_(k, element_key))
        return &element;
    }
    return nullptr;
  }

  void clear() {
    auto b = get_buckets();
    assert(b);
    b->clear();
  }

  void insert(T& val) {
    auto b = get_buckets();
    assert(b);

    auto& key = val.*keyptr;
    auto hash = hash_fn_(key);
    auto index = hash % b->size();
    auto& bucket = b->get_bucket(index);
    bucket.push_front(val);
  }

  static void erase(T& val) { RcuHList<T, hookptr>::erase(val); }

  Future<void> resize(uint8_t buckets_shift) {
    auto new_sz = 1 << buckets_shift;
    auto buckets = get_buckets();
    auto old_sz = buckets->size();

    if (new_sz < old_sz) {
      /* shrink */

      // create new buckets from old buckets
      auto new_buckets = buckets_t::Create(*buckets, new_sz);
      // for each new bucket, make sure all old buckets which map to it, are
      // chained
      for (size_t i = 0; i < new_sz; ++i) {
        auto last_bucket = &buckets->get_bucket(i);
        for (size_t j = i + new_sz; j < old_sz; j += new_sz) {
          auto& this_bucket = buckets->get_bucket(j);
          // skip over empty buckets
          if (this_bucket.empty())
            continue;

          last_bucket->splice_end(this_bucket.front());
          last_bucket = &this_bucket;
        }
      }
      std::atomic_thread_fence(std::memory_order_release);
      // The chaining does not carry a data dependency with a write to buckets_,
      // so because we load with memory_order_consume, we need to wait for a
      // grace period before writing to buckets_
      return CallRcu([this, new_buckets]() {
        // Now the new bucket is initialized and all readers are guaranteed to
        // see the effect of the chaining so we can write the new bucket pointer
        auto old_buckets = buckets_.exchange(new_buckets);
        return CallRcu([this, old_buckets]() {
          // We are now guaranteed that all readers have seen the new bucket
          buckets_t::Destroy(old_buckets);
        });
      });
    } else if (new_sz > old_sz) {
      /* grow */
      // create new buckets
      auto new_buckets = buckets_t::Create(new_sz);
      // Each new bucket has only one old bucket where its elements might be.
      // For each new bucket, find the first element in the old bucket (if it
      // exists) and set it as the front of the new bucket
      for (size_t i = 0; i < new_sz; ++i) {
        for (auto& node : buckets->get_bucket(i % old_sz)) {
          auto& key = node.*keyptr;
          auto hash = hash_fn_(key);
          if ((hash % new_sz) == i) {
            new_buckets->get_bucket(i).set(node);
            break;
          }
        }
      }

      auto old_buckets = buckets_.exchange(new_buckets);
      std::atomic_thread_fence(std::memory_order_release);
      auto unzipper = new Unzipper(old_buckets, new_sz);
      return CallRcu([unzipper]() {
        // everyone is now working off the new buckets, start unzipping them
        return unzipper->unzip();
      });
    }
    return MakeReadyFuture<void>();
  }
};
}  // namespace ebbrt

#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_RCUTABLE_H_
