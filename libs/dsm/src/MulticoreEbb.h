//          Copyright Boston University SESA Group 2013 - 2014.
//

//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef MULTICOREEBB_H_
#define MULTICOREEBB_H_

#include <cassert>
#include <mutex>
#include <utility>
#include <cstdlib>
#include <boost/container/flat_map.hpp>
#include <string>
#include <ebbrt/Cpu.h>
#include <ebbrt/Debug.h>
#include <ebbrt/EbbAllocator.h>
#include <ebbrt/EbbId.h>
#include <ebbrt/EbbRef.h>
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/SpinLock.h>

#include <type_traits>

/*
 * MulticoreEbb class template defines a multicore Ebb class (i.e, one rep per
 * core/context) with an optional shared root object. The (2) templates
 * arguments
 * specify the base rep and root class types.
 *
 * class MyEbb : MulticoreEbb<MyEbb, MyEbbRoot>
 *
 * A rep of the multicore Ebb contains a list of the other
 * reps local to the node, as well as a pointer to shared root object.
 *
 * A Create() function is used to construct the initial ref and format the
 * local_id_map entry.
 */
namespace ebbrt {
namespace detail {
template <class T> using RepMap = boost::container::flat_map<size_t, T *>;
}
using detail::RepMap;

/* forward declarations of class templates */
template <class T, class R> class MulticoreEbbRoot;
template <class T, class R> class MulticoreEbb;

/* Ebb Root class templace with R-type Rep Map */
template <class T, class R> class MulticoreEbbRoot {
public:
  MulticoreEbbRoot(EbbId id) : id_(id){};

protected:
  static void HandleFault(EbbId id);
  // Derived class can re-implement protected methods, which hides
  // (doesn't conflict with) these base class implementations.
  R *create_rep() { 
    auto rep = new R(id_);
    rep->SetRoot(static_cast<T *>(this));
    return  rep;
  };
  R *create_initial_rep() { return create_rep(); };
  R *cache_rep_(size_t core);
  SpinLock lock_;
  RepMap<R> reps_;
  EbbId id_;

private:
  friend class MulticoreEbb<R, T>;
};

/* Multicore Ebb class template with typed Root */
template <class T, class R> class MulticoreEbb {
//  static_assert(std::is_base_of<MulticoreEbbRoot<R, T>, R>::value,
//                "Root type must inherit from MulticoreEbbRoot<R,T>");

public:
  MulticoreEbb() = delete; // Must ber constructed w/ root pointer
  MulticoreEbb(ebbrt::EbbId id) : id_(id) {};
  void SetRoot(R * root) { root_ = root; };
  static T &HandleFault(EbbId id);

protected:
  R *root_;
  EbbId id_;
private:
  // By making the base root type a friend allows the rep constructor to
  // initialize protected members
  friend class MulticoreEbbRoot<R, T>;
};

template <class T, class R> R *MulticoreEbbRoot<T, R>::cache_rep_(size_t core) {
  auto it = reps_.find(core);
  if (it != reps_.end()) {
    // rep was already cached for this core, return it
    return it->second;
  } else {
    //  construct a new rep and cache the address
    R *rep;
    if (reps_.size() == 0) {
      // construct initial rep on the node
      rep = create_initial_rep();
    } else {
      rep = create_rep();
    }
    // cached rep
    {
      std::lock_guard<ebbrt::SpinLock> guard(lock_);
      reps_[core] = rep;
    }
    return rep;
  }
}

template <class T, class R>
void MulticoreEbbRoot<T, R>::HandleFault(EbbId id) {
  // Default behavior is to construct and cache a local root object
  LocalIdMap::Accessor accessor;
  auto created = local_id_map->Insert(accessor, id);
  // In this case of a race, the thread that successfully wrote to
  // the local_id_map will create a new root object
  if (created) {
    ebbrt::kprintf("New root #%d\n", id);
    accessor->second = new T(id);
  }

}

template <class T, class R> T &MulticoreEbb<T, R>::HandleFault(EbbId id) {
retry : {

  // Check for root in LocalIdMap (read-lock)
  LocalIdMap::ConstAccessor accessor;
  auto found = local_id_map->Find(accessor, id);
  if (found) {
    // Ask root for representative (remain locked)
    // Let root check for exiting rep and constructs one if needed
    auto root = boost::any_cast<R *>(accessor->second);
    T *rep = root->cache_rep_((size_t)Cpu::GetMine());
    EbbRef<T>::CacheRef(id, *rep);
    return *rep;
  }
}
  // No root was found on this node. Trigger global miss for this root type
  MulticoreEbbRoot<R, T>::HandleFault(id);
  // retry, expecting we'll find the root on the next pass
  goto retry;
}

} // namespace ebbrt

#endif // COMMON_SRC_INCLUDE_EBBRT_MULTICOREEBB_H_
