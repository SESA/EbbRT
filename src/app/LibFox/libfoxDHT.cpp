/*
  EbbRT: Distributed, Elastic, Runtime
  Copyright (C) 2013 SESA Group, Boston University

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "mpi.h"

#include <string>
#include <thread>

extern "C" {
#include "app/LibFox/libfox.h"
}

#include "ebbrt.hpp"
#include "ebb/HashTable/HashTable.hpp"
#include "ebb/MessageManager/MessageManager.hpp"

//static hashtable for these apps
const ebbrt::EbbRef<ebbrt::HashTable> the_hash_table =
  ebbrt::EbbRef<ebbrt::HashTable>(ebbrt::lrt::config::
                                  find_static_ebb_id("HashTable"));

struct fox_st {
  fox_st(int numproc, int myprocid) : table{the_hash_table},
         context{instance}, nprocs{numproc}, procid{myprocid}
  {
    context.Activate();
    ebbrt::message_manager->StartListening();
    if (MPI_Barrier(MPI_COMM_WORLD) != MPI_SUCCESS) {
      throw std::runtime_error("MPI_Barrier failed");
    }
    context.Deactivate();
    can_loop = true;
    done_loop = true;
    quit = false;
    listener = std::thread([&]() {
        while (1) {
          std::unique_lock<std::mutex> lk{loop_lock};
          loop_cv.wait(lk, [&]{return can_loop;});
          if (quit) {
            return;
          }
          //fprintf(stderr, "listener START\n");
          context.Activate();
          done_loop = false;
          ebbrt::event_manager->Async([&]{lk.unlock();});
          context.Loop(-1);
          //fprintf(stderr, "listener STOP\n");
          context.Deactivate();
          lk.lock();
          done_loop = true;
          loop_cv.notify_one();
        }
      });
    listener.detach();
  }

  ebbrt::EbbRef<ebbrt::HashTable> table;
  ebbrt::EbbRT instance;
  ebbrt::Context context;
  std::thread listener;
  std::mutex loop_lock;
  std::condition_variable loop_cv;
  bool can_loop;
  bool done_loop;
  bool quit;
  int nprocs;
  int procid;
};

namespace {
  void stop_listener(fox_ptr fhand) {
    //fprintf(stderr, "stop_listener: ENTER\n");
    std::unique_lock<std::mutex> lk(fhand->loop_lock);
    fhand->can_loop = false;
    fhand->context.Break();
    fhand->loop_cv.wait(lk, [&]{return fhand->done_loop;});
    //fprintf(stderr, "stop_listener: LEAVE\n");
  }

  void start_listener(fox_ptr fhand) {
    //fprintf(stderr, "start_listener: ENTER\n");
    {
      std::lock_guard<std::mutex> lk(fhand->loop_lock);
      fhand->can_loop = true;
    }
    fhand->loop_cv.notify_one();
    //fprintf(stderr, "start_listener: LEAVE\n");
  }

  void kill_listener(fox_ptr fhand) {
    stop_listener(fhand);
    fhand->quit = true;
    start_listener(fhand);
  }
}

extern "C"
int
fox_new(fox_ptr* fhand_ptr, int nprocs, int procid)
{
  *fhand_ptr = new fox_st(nprocs, procid);
  return 0;
}

extern "C"
int
fox_free(fox_ptr fhand)
{
  if (MPI_Barrier(MPI_COMM_WORLD) != MPI_SUCCESS) {
    throw std::runtime_error("MPI_Barrier failed");
  }
  kill_listener(fhand);
  delete fhand;
  return 0;
}

extern "C"
int
fox_flush(fox_ptr fhand, int term)
{
  return 0;
}

extern "C"
int
fox_server_add(fox_ptr fhand, const char *hostlist)
{
  return 0;
}

extern "C"
int
fox_set(fox_ptr fhand,
        const char *key, size_t key_sz,
        const char *value, size_t value_sz)
{
  stop_listener(fhand);

  fhand->context.Activate();
  fhand->table->Set(key, key_sz,
                    value, value_sz);
  fhand->context.Deactivate();

  start_listener(fhand);
  return 0;
}

extern "C"
int
fox_get(fox_ptr fhand,
        const char *key, size_t key_sz,
        char **pvalue, size_t *pvalue_sz)
{
  stop_listener(fhand);

  fhand->context.Activate();
  *pvalue = nullptr;
  *pvalue_sz = 0;
  auto fut = fhand->table->Get(key, key_sz);
  fut.OnSuccess([&](std::pair<char*, size_t> pair) {
      auto& val = pair.first;
      auto& size = pair.second;
      assert(size != 0);
      *pvalue = val;
      *pvalue_sz = size;
      fhand->context.Break();
    });
  fhand->context.Loop(-1);
  //We only break out because the callback executed
  start_listener(fhand);
  return 0;
}

extern "C"
int
fox_delete(fox_ptr fhand, const char* key, size_t key_sz)
{
  assert(0);
  return 0;
}

extern "C"
int
fox_sync_set(fox_ptr fhand, unsigned delta,
             const char* key, size_t key_sz,
             const char* value, size_t value_sz)
{
  stop_listener(fhand);

  fhand->context.Activate();
  fhand->table->SyncSet(key, key_sz,
                        value, value_sz,
                        delta);
  fhand->context.Deactivate();

  start_listener(fhand);
  return 0;
}

extern "C"
int
fox_sync_get(fox_ptr fhand, unsigned delta,
             const char *key, size_t key_sz,
             char **pvalue, size_t *pvalue_sz)
{
  stop_listener(fhand);

  fhand->context.Activate();
  *pvalue = nullptr;
  *pvalue_sz = 0;
  auto fut = fhand->table->SyncGet(key, key_sz, delta);
  fut.OnSuccess([&](std::pair<char*, size_t> pair) {
      auto& val = pair.first;
      auto& size = pair.second;
      assert(size != 0);
      *pvalue = val;
      *pvalue_sz = size;
      fhand->context.Break();
    });
  fhand->context.Loop(-1);
  //We only break out because the callback executed
  start_listener(fhand);
  return 0;
}

extern "C"
int
fox_broad_set(fox_ptr fhand,
              const char *key, size_t key_sz,
              const char *value, size_t value_sz)
{
  return fox_sync_set(fhand, 1, key, key_sz, value, value_sz);
}

extern "C"
int
fox_broad_get(fox_ptr fhand,
              const char *key, size_t key_sz,
              char **pvalue, size_t *pvalue_sz)
{
  return fox_sync_get(fhand, 1, key, key_sz, pvalue, pvalue_sz);
}

extern "C"
int
fox_queue_set(fox_ptr fhand,
              const char *key, size_t key_sz,
              const char *value, size_t value_sz)
{
  std::string str{key, key_sz};
  str += "_B";

  stop_listener(fhand);
  fhand->context.Activate();
  uint32_t qidx = 0xFFFFFFFF;
  auto fut = fhand->table->Increment(str.c_str(), str.length());
  fut.OnSuccess([&](uint32_t val) {
      qidx = val;
      fhand->context.Break();
    });
  fhand->context.Loop(-1);
  //We only break out because the callback executed
  start_listener(fhand);

  std::string str2{key, key_sz};
  str2 += "_";
  str2 += std::to_string(qidx);
  fox_sync_set(fhand, 1, str2.c_str(), str2.length(), value, value_sz);

  return 0;
}

extern "C"
int
fox_queue_get(fox_ptr fhand,
              const char *key, size_t key_sz,
              char **pvalue, size_t *pvalue_sz)
{
  std::string str{key, key_sz};
  str += "_F";

  stop_listener(fhand);
  fhand->context.Activate();
  uint32_t qidx = 0xFFFFFFFF;
  auto fut = fhand->table->Increment(str.c_str(), str.length());
  fut.OnSuccess([&](uint32_t val) {
      qidx = val;
      fhand->context.Break();
    });
  fhand->context.Loop(-1);
  //we only break out because the callback executed
  start_listener(fhand);

  std::string str2{key, key_sz};
  str2 += "_";
  str2 += std::to_string(qidx);
  fox_sync_get(fhand, 1, str2.c_str(), str2.length(), pvalue, pvalue_sz);

  return 0;
}

extern "C"
int
fox_broad_queue_set(fox_ptr fhand,
                    const char *key, size_t key_sz,
                    const char *value, size_t value_sz)
{
  std::string str {key, key_sz};

  for (int idx = 0; idx < fhand->nprocs; ++idx) {
    auto newkey = str;
    newkey += std::to_string(idx);
    fox_queue_set(fhand, newkey.c_str(), newkey.length(),
                  value, value_sz);
  }
  return 0;
}

extern "C"
int
fox_dist_queue_set(fox_ptr fhand,
                   const char *key, size_t key_sz,
                   const char *value, size_t value_sz)
{
  static int oproc = 0;

  std::string str{key, key_sz};
  str += std::to_string((fhand->procid + oproc) % fhand->nprocs);
  fox_queue_set(fhand, str.c_str(), str.length(), value, value_sz);

  oproc = (oproc + 1) % fhand->nprocs;
  return 0;
}

extern "C"
int
fox_dist_queue_get(fox_ptr fhand,
                   const char *key, size_t key_sz,
                   char **pvalue, size_t *pvalue_sz)
{
  std::string str{key, key_sz};

  str += std::to_string(fhand->procid);

  return fox_queue_get(fhand, str.c_str(), str.length(), pvalue, pvalue_sz);
}

extern "C"
int
fox_reduce_set(fox_ptr fhand,
               const char *key, size_t key_sz,
               const char *value, size_t value_sz)
{
  std::string str{key, key_sz};

  str += "_";
  str += std::to_string(fhand->procid);

  return fox_sync_set(fhand, 1, str.c_str(), str.length(), value, value_sz);
}

extern "C"
int
fox_reduce_get(fox_ptr fhand,
               const char *key, size_t key_sz,
               char *pvalue, size_t pvalue_sz,
               void (*reduce)(void *out, void *in))
{
  for (int idx = 0; idx < fhand->nprocs; ++idx) {
    std::string str{key, key_sz};
    str += "_";
    str += std::to_string(idx);

    char* data_ptr;
    size_t data_sz;
    fox_sync_get(fhand, 1, str.c_str(), str.length(), &data_ptr, &data_sz);

    reduce(pvalue, data_ptr);
    free(data_ptr);
  }

  return 0;
}

extern "C"
int
fox_collect(fox_ptr fhand,
            const char *key, size_t key_sz,
            int root, int opt)
{
  static int val = 0;
  std::string str{key, key_sz};
  str += "_";
  str += std::to_string(val);
  val++;

  char* buf_ptr = reinterpret_cast<char*>(&fhand->procid);
  size_t buf_sz = sizeof(fhand->procid);
  fox_sync_set(fhand, 1, str.c_str(), str.length(), buf_ptr, buf_sz);

  fox_sync_get(fhand, fhand->nprocs, str.c_str(), str.length(),
               &buf_ptr, &buf_sz);
  free(buf_ptr);
  return 0;
}
