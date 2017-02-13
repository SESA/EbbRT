//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef VFS_H_
#define VFS_H_

#include <atomic>
#include <ebbrt/CacheAligned.h>
#include <ebbrt/Future.h>
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/SharedIOBufRef.h>
#include <ebbrt/SpinLock.h>
#include <ebbrt/StaticSharedEbb.h>
#include <ebbrt/UniqueIOBuf.h>
#include <unordered_map>

namespace ebbrt {

class Vfs : public ebbrt::StaticSharedEbb<Vfs>, public CacheAligned {
 public:
  class Fd {
   public:
    class Root {
     public:
      virtual Fd& HandleFault(EbbId id) = 0;

     private:
      friend class Fd;
    };
    static Fd& HandleFault(EbbId id) {
      LocalIdMap::ConstAccessor accessor;
      auto found = local_id_map->Find(accessor, id);
      if (!found)
        throw std::runtime_error("Failed to find root for Fd");
      auto rep = boost::any_cast<Root*>(accessor->second);
      return rep->HandleFault(id);
    };
    virtual ~Fd(){};
    virtual ebbrt::Future<std::unique_ptr<IOBuf>> Read(size_t len) = 0;
    virtual ebbrt::Future<uint8_t> Close() = 0;
    // virtual void IsReady() = 0;
    // virtual void Lseek() = 0;
    virtual void Write(std::unique_ptr<IOBuf>) = 0;
    virtual uint32_t GetFlags() = 0;
    virtual void SetFlags(uint32_t) = 0;
  };
  explicit Vfs() : fd_{0} {};
  int RegisterFd(ebbrt::EbbRef<ebbrt::Vfs::Fd>);
  ebbrt::EbbRef<ebbrt::Vfs::Fd> Lookup(int fd_int);

 private:
  std::atomic_int fd_;
  std::unordered_map<int, ebbrt::EbbRef<ebbrt::Vfs::Fd>> descriptor_map_;
};

static const auto root_vfs = EbbRef<Vfs>();

}  // namespace ebbrt
#endif  // VFS_H_
