//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <ebbrt/Message.h>
#include <ebbrt/SharedEbb.h>

class FileSystem : public ebbrt::SharedEbb<FileSystem>,
                   public ebbrt::Messagable<FileSystem> {
public:
  struct StatInfo {
    uint64_t stat_dev;
    uint64_t stat_ino;
    uint64_t stat_mode;
    uint64_t stat_nlink;
    uint64_t stat_uid;
    uint64_t stat_gid;
    uint64_t stat_rdev;
    uint64_t stat_size;
    uint64_t stat_atime;
    uint64_t stat_mtime;
    uint64_t stat_ctime;
  };

  FileSystem(ebbrt::EbbId id, ebbrt::Messenger::NetworkId frontend_id);

  static ebbrt::Future<void> PreCreate(ebbrt::EbbId id);
  static FileSystem &CreateRep(ebbrt::EbbId id);
  static void DestroyRep(ebbrt::EbbId id, FileSystem &rep);
  ebbrt::Future<StatInfo> Stat(const char *path);
  ebbrt::Future<StatInfo> LStat(const char *path);
  ebbrt::Future<StatInfo> FStat(int fd);
  ebbrt::Future<std::string> GetCwd();
  ebbrt::Future<int> Open(const char *path, int flags, int mode);
  ebbrt::Future<std::string> Read(int fd, size_t length, int64_t offset);

private:
  void ReceiveMessage(ebbrt::Messenger::NetworkId nid,
                      std::unique_ptr<ebbrt::IOBuf> &&buffer);

  ebbrt::Messenger::NetworkId frontend_id_;
  std::mutex lock_;
  uint32_t request_;
  std::unordered_map<uint32_t, ebbrt::Promise<StatInfo> > stat_promises_;
  std::unordered_map<uint32_t, ebbrt::Promise<std::string> > string_promises_;
  std::unordered_map<uint32_t, ebbrt::Promise<int> > open_promises_;

  friend ebbrt::Messagable<FileSystem>;
};

#endif // FILESYSTEM_H_
