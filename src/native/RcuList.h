//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef BAREMETAL_SRC_INCLUDE_EBBRT_RCULIST_H_
#define BAREMETAL_SRC_INCLUDE_EBBRT_RCULIST_H_

#include <cassert>

#include <boost/intrusive/parent_from_member.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include "Rcu.h"

namespace ebbrt {

struct RcuHListHook {
  std::atomic<RcuHListHook*> next;
  std::atomic<std::atomic<RcuHListHook*>*> pprev;
};

template <typename T, RcuHListHook T::*hookptr> class RcuHList {
  std::atomic<RcuHListHook*> head_;

 public:
  template <typename Value>
  class iter : public boost::iterator_facade<iter<Value>, Value,
                                             boost::forward_traversal_tag> {
    struct enabler {};

   public:
    iter() : node_(nullptr) {}
    explicit iter(T* p) : node_(p) {}

    template <typename OtherValue>
    iter(
        const iter<OtherValue>& other,
        typename std::enable_if<std::is_convertible<OtherValue*, Value*>::value,
                                enabler>::type = enabler())
        : node_(other.node_) {}

   private:
    friend class boost::iterator_core_access;

    void increment() {
      auto& hook = node_->*hookptr;
      auto next = hook.next.load(std::memory_order_consume);
      if (!next) {
        node_ = nullptr;
      } else {
        node_ = ::boost::intrusive::get_parent_from_member(next, hookptr);
      }
    }

    template <typename OtherValue>
    bool equal(const iter<OtherValue>& other) const {
      return this->node_ == other.node_;
    }

    T& dereference() const { return *node_; }

    T* node_;
  };

  typedef iter<T> iterator;
  typedef iter<const T> const_iterator;

  RcuHList() { head_.store(nullptr, std::memory_order_relaxed); }

  RcuHList& operator=(const RcuHList& other) {
    head_.store(other.head_.load(std::memory_order_consume),
                std::memory_order_relaxed);
    return *this;
  }

  iterator begin() {
    auto head_hook = head_.load(std::memory_order_consume);
    if (head_hook == nullptr)
      return end();
    auto head_ptr =
        ::boost::intrusive::get_parent_from_member(head_hook, hookptr);
    return iterator(head_ptr);
  }
  iterator end() { return iterator(nullptr); }
  const_iterator cbegin() const {
    auto head_hook = head_.load(std::memory_order_consume);
    if (head_hook == nullptr)
      return cend();
    auto head_ptr =
        ::boost::intrusive::get_parent_from_member(head_hook, hookptr);
    return const_iterator(head_ptr);
  }
  const_iterator cend() const { return const_iterator(nullptr); }

  T& front() { return *begin(); }

  bool empty() const { return cbegin() == cend(); }

  void clear() { head_.store(nullptr, std::memory_order_relaxed); }

  static void clear_after(T& node) {
    auto& hook = node.*hookptr;
    hook.next.store(nullptr, std::memory_order_release);
  }

  static void clear_after(iterator it) { clear_after(*it); }

  void push_front(T& node) {
    auto& hook = node.*hookptr;
    auto head = head_.load(std::memory_order_relaxed);

    hook.next.store(head, std::memory_order_relaxed);
    hook.pprev.store(&head_, std::memory_order_relaxed);
    head_.store(&hook, std::memory_order_release);
    if (head) {
      head->pprev.store(&hook.next, std::memory_order_relaxed);
    }
  }

  void insert_before(T& new_node, T& node) {
    auto& new_hook = new_node.*hookptr;
    auto& hook = node.*hookptr;
    auto pprev = hook.pprev.load(std::memory_order_relaxed);
    new_hook.pprev.store(pprev, std::memory_order_relaxed);
    new_hook.next.store(&hook, std::memory_order_relaxed);
    pprev->store(&new_hook, std::memory_order_release);
    hook.pprev.store(&new_hook.next, std::memory_order_relaxed);
  }

  void insert_before(iterator it, T& node) { insert_before(*it, node); }

  void insert_after(T& new_node, T& node) {
    auto& new_hook = new_node.*hookptr;
    auto& hook = node.*hookptr;
    auto hook_next = hook.next.load(std::memory_order_relaxed);
    new_hook.next.store(hook_next, std::memory_order_relaxed);
    new_hook.pprev.store(&hook.next, std::memory_order_relaxed);
    hook.next.store(&new_hook, std::memory_order_release);
    if (hook_next)
      hook_next->pprev.store(&new_hook.next, std::memory_order_relaxed);
  }

  void insert_after(T& new_node, iterator it) { insert_after(new_node, *it); }

  void splice_end(T& new_node) {
    assert(!empty());
    auto it = begin();
    while (std::next(it) != end())
      ++it;
    auto& end_node = *it;
    auto& new_hook = new_node.*hookptr;
    auto& end_hook = end_node.*hookptr;
    new_hook.pprev.store(&end_hook.next, std::memory_order_relaxed);
    end_hook.next.store(&new_hook, std::memory_order_release);
  }

  static void splice_after(T& new_node, T& node) {
    auto& new_hook = new_node.*hookptr;
    auto& hook = node.*hookptr;
    new_hook.pprev.store(&hook.next, std::memory_order_relaxed);
    hook.next.store(&new_hook, std::memory_order_release);
  }

  static void splice_after(T& new_node, iterator node) {
    return splice_after(new_node, *node);
  }

  void set(T& new_node) {
    auto& hook = new_node.*hookptr;
    head_.store(&hook, std::memory_order_release);
  }

  static void erase(T& node) {
    auto& hook = node.*hookptr;
    auto next = hook.next.load(std::memory_order_relaxed);
    auto pprev = hook.pprev.load(std::memory_order_relaxed);
    *pprev = next;
    if (next)
      next->pprev.store(pprev, std::memory_order_relaxed);
  }

  static void erase(iterator i) { erase(*i); }
};

}  // namespace ebbrt
#endif  // BAREMETAL_SRC_INCLUDE_EBBRT_RCULIST_H_
