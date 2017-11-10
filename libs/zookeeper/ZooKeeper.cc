//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
#include "ZooKeeper.h"

#ifndef __ebbrt__
#include <chrono>
#include <thread>

/* When executed on hosted Create will spawn a new ZooKeeper container. */
ebbrt::EbbRef<ebbrt::ZooKeeper>
ebbrt::ZooKeeper::Create(EbbId id, Watcher* connection_watcher, int timeout_ms,
                         int timer_ms, std::string server_hosts) {
  if (server_hosts.empty()) {
    // start a ZooKeeper server instance on our network
    server_hosts = node_allocator->AllocateContainer(
        "jplock/zookeeper", "--expose 2888 --expose 3888 --expose 2181");
    server_hosts += std::string(":2181");
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(5s);
  }
  node_allocator->AppendArgs(std::string("zookeeper=") + server_hosts);
  auto rep = new ebbrt::ZooKeeper(server_hosts, connection_watcher, timeout_ms,
                                  timer_ms);
  local_id_map->Insert(std::make_pair(id, std::move(rep)));
  return ebbrt::EbbRef<ebbrt::ZooKeeper>(id);
}
#else

/* When executed on native Create checks command line args for ZK server. */
ebbrt::EbbRef<ebbrt::ZooKeeper>
ebbrt::ZooKeeper::Create(EbbId id, Watcher* connection_watcher, int timeout_ms,
                         int timer_ms, std::string server_hosts) {
  if (server_hosts.empty()) {
    // retrieve  ZooKeeper ip/port from the command line
    auto cl = std::string(ebbrt::multiboot::CmdLine());
    auto zkstr = std::string("zookeeper=");
    auto loc = cl.find(zkstr);
    if (loc == std::string::npos) {
      kabort("No configuration for ZooKeeper found in command line\n");
    }
    server_hosts = cl.substr((loc + zkstr.size()));
    auto gap = server_hosts.find(";");
    if (gap != std::string::npos) {
      server_hosts = server_hosts.substr(0, gap);
    }
  }
  auto rep = new ebbrt::ZooKeeper(server_hosts, connection_watcher, timeout_ms,
                                  timer_ms);
  local_id_map->Insert(std::make_pair(id, std::move(rep)));
  return ebbrt::EbbRef<ebbrt::ZooKeeper>(id);
}
#endif

ebbrt::ZooKeeper::ZooKeeper(const std::string& server_hosts,
                            ebbrt::ZooKeeper::Watcher* connection_watcher,
                            int timeout_ms, int timer_ms)
    : connection_watcher_(connection_watcher) {

  zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);
  zoo_deterministic_conn_order(1);  // deterministic command->server assignment
  zk_ = zookeeper_init(server_hosts.c_str(), event_completion, timeout_ms,
                       nullptr, connection_watcher, 0);

  timer->Start(*this, std::chrono::milliseconds(timer_ms * 2), true);
  return;
}

ebbrt::ZooKeeper::~ZooKeeper() {
  if (zk_) {
    auto ret = zookeeper_close(zk_);
    if (ret != ZOK) {
      ebbrt::kprintf("ZooKeeper close error: %d\n", ret);
    }
  }
}

/* ZooKeeper IO Handler */
void ebbrt::ZooKeeper::Fire() {
  std::lock_guard<ebbrt::SpinLock> guard(lock_);
  struct pollfd fds[1];

  if (zk_) {
    struct timeval tv;
    int fd;
    int interest;
    int timeout;
    int maxfd = 1;
    int rc;

    rc = zookeeper_interest(zk_, &fd, &interest, &tv);
    if (rc != ZOK) {
      ebbrt::kabort("zookeeper_interest error");
    }
    if (fd != -1) {
      fds[0].fd = fd;
      fds[0].events = (interest & ZOOKEEPER_READ) ? POLLIN : 0;
      fds[0].events |= (interest & ZOOKEEPER_WRITE) ? POLLOUT : 0;
      maxfd = 1;
      fds[0].revents = 0;
    }
    timeout = tv.tv_sec * 1000 + (tv.tv_usec / 1000);

    poll(fds, maxfd, timeout);
    if (fd != -1) {
      interest = (fds[0].revents & POLLIN) ? ZOOKEEPER_READ : 0;
      interest |= ((fds[0].revents & POLLOUT) || (fds[0].revents & POLLHUP))
                      ? ZOOKEEPER_WRITE
                      : 0;
    }
    zookeeper_process(zk_, interest);
    if (rc != ZOK) {
      ebbrt::kabort("zookeeper_process error");
    }
    if (is_unrecoverable(zk_)) {
      //  api_epilog(zk_, 0);
      ebbrt::kabort(("zookeeper io handler  terminated"));
    }
  }
}

ebbrt::Future<ebbrt::ZooKeeper::Znode>
ebbrt::ZooKeeper::New(const std::string& path, const std::string& value,
                      ZooKeeper::Flag flags) {

  auto p = new ebbrt::Promise<Znode>;
  auto f = p->GetFuture();

  // TODO(jmc): verify this work for empty string (i.e., c_str() = "" or
  // nullptr)
  zoo_acreate(zk_, path.c_str(), value.c_str(), value.size(),
              &ZOO_OPEN_ACL_UNSAFE, static_cast<int>(flags), string_completion,
              p);
  return f;
}

ebbrt::Future<ebbrt::ZooKeeper::Znode>
ebbrt::ZooKeeper::Stat(const std::string& path,
                       ebbrt::ZooKeeper::Watcher* watcher) {
  auto p = new ebbrt::Promise<Znode>;
  auto f = p->GetFuture();

  if (watcher) {
    zoo_awexists(zk_, path.c_str(), event_completion, watcher, stat_completion,
                 p);
  } else {
    zoo_aexists(zk_, path.c_str(), 0, stat_completion, p);
  }
  return f;
}

ebbrt::Future<bool>
ebbrt::ZooKeeper::Exists(const std::string& path,
                         ebbrt::ZooKeeper::Watcher* watcher) {
  auto p = new ebbrt::Promise<bool>;
  auto f = p->GetFuture();
  if (validate_path(path)) {
    ebbrt::kabort("ZooKeeper: invalid path %s\n", path.c_str());
  }
  Stat(path, watcher).Then([p](ebbrt::Future<ebbrt::ZooKeeper::Znode> z) {
    auto znode = z.Get();
    if (znode.err == ZOK) {
      p->SetValue(true);
    } else {
      p->SetValue(false);
    }
  });
  return f;
}

ebbrt::Future<ebbrt::ZooKeeper::Znode>
ebbrt::ZooKeeper::Get(const std::string& path,
                      ebbrt::ZooKeeper::Watcher* watcher) {
  auto p = new ebbrt::Promise<Znode>;
  auto f = p->GetFuture();

  if (watcher) {
    zoo_awget(zk_, path.c_str(), event_completion, watcher, data_completion, p);
  } else {
    zoo_aget(zk_, path.c_str(), 0, data_completion, p);
  }
  return f;
}

ebbrt::Future<ebbrt::ZooKeeper::ZnodeChildren>
ebbrt::ZooKeeper::GetChildren(const std::string& path,
                              ebbrt::ZooKeeper::Watcher* watcher) {
  auto p = new ebbrt::Promise<ZnodeChildren>;
  auto f = p->GetFuture();

  if (watcher) {
    zoo_awget_children2(zk_, path.c_str(), event_completion, watcher,
                        strings_completion, p);
  } else {
    zoo_aget_children2(zk_, path.c_str(), 0, strings_completion, p);
  }
  return f;
}

ebbrt::Future<ebbrt::ZooKeeper::Znode>
ebbrt::ZooKeeper::Delete(const std::string& path, int version) {
  auto p = new ebbrt::Promise<Znode>;
  auto f = p->GetFuture();

  zoo_adelete(zk_, path.c_str(), version, void_completion, p);
  return f;
}

ebbrt::Future<ebbrt::ZooKeeper::Znode>
ebbrt::ZooKeeper::Set(const std::string& path, const std::string& value,
                      int version) {
  auto p = new ebbrt::Promise<Znode>;
  auto f = p->GetFuture();

  zoo_aset(zk_, path.c_str(), value.c_str(), value.size(), version,
           stat_completion, p);
  return f;
}

void ebbrt::ZooKeeper::event_completion(zhandle_t* h, int type, int state,
                                        const char* path, void* ctx) {
  auto watcher = static_cast<Watcher*>(ctx);
  watcher->WatchHandler(type, state, path);
}

void ebbrt::ZooKeeper::stat_completion(int rc,
                                       const ebbrt::ZooKeeper::ZkStat* stat,
                                       const void* data) {
  Znode res;
  res.err = rc;
  if (stat)
    res.stat = *stat;
  auto p = static_cast<ebbrt::Promise<Znode>*>(const_cast<void*>(data));
  p->SetValue(std::move(res));
  delete p;
  return;
}

void ebbrt::ZooKeeper::string_completion(int rc, const char* name,
                                         const void* data) {
  Znode res;
  res.err = rc;
  if (name)
    res.value = std::string(name);
  auto p = static_cast<ebbrt::Promise<Znode>*>(const_cast<void*>(data));
  p->SetValue(std::move(res));
  delete p;
  return;
}

void ebbrt::ZooKeeper::data_completion(int rc, const char* value, int value_len,
                                       const ebbrt::ZooKeeper::ZkStat* stat,
                                       const void* data) {
  Znode res;
  res.err = rc;
  if (stat)
    res.stat = *stat;
  if (value_len > 0)
    res.value = std::string(value, static_cast<size_t>(value_len));
  auto p = static_cast<ebbrt::Promise<Znode>*>(const_cast<void*>(data));
  p->SetValue(std::move(res));
  delete p;
  return;
}

void ebbrt::ZooKeeper::strings_completion(int rc,
                                          const struct String_vector* strings,
                                          const ebbrt::ZooKeeper::ZkStat* stat,
                                          const void* data) {
  ZnodeChildren res;
  res.err = rc;
  res.stat = *stat;
  if (strings) {
    for (int i = 0; i < strings->count; ++i) {
      res.values.emplace_back(std::string(strings->data[i]));
    }
  }
  auto p = static_cast<ebbrt::Promise<ZnodeChildren>*>(const_cast<void*>(data));
  p->SetValue(std::move(res));
  delete p;
  return;
}

void ebbrt::ZooKeeper::void_completion(int rc, const void* data) {
  Znode res;
  res.err = rc;
  auto p = static_cast<ebbrt::Promise<Znode>*>(const_cast<void*>(data));
  p->SetValue(std::move(res));
  delete p;
  return;
}
