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

#include <cstdlib>
#include <cstring>

#include <string>
#include <vector>

extern "C" {
#include "src/app/LibFox/libfox.h"
}

#include "src/ebb/HashTable/LocalHashTable.hpp"

struct fox_st {
  fox_st(int nprocs_, int procid_) : nprocs{nprocs_}, procid{procid_} {}
  std::vector<std::string> hosts;
  std::unordered_multimap<std::string, std::string> queue;
  std::unordered_map<std::string, std::string> table;
  int nprocs;
  int procid;
};

extern "C"
int
fox_new(fox_ptr* fhand_ptr, int nprocs, int procid)
{
  try {
    *fhand_ptr = new fox_st(nprocs, procid);
  } catch (...) {
    return -1;
  }
  return 0;
}

extern "C"
int
fox_free(fox_ptr fhand)
{
  delete fhand;
  return 0;
}

extern "C"
int
fox_flush(fox_ptr fhand, int term)
{
  fhand->queue.clear();
  fhand->table.clear();

  return 0;
}

extern "C"
int
fox_server_add(fox_ptr fhand, const char *hostlist)
{
  fhand->hosts.emplace_back(hostlist);
  return 0;
}

extern "C"
int
fox_set(fox_ptr fhand,
        const char *key, size_t key_sz,
        const char *value, size_t value_sz)
{
  fhand->table.emplace(std::piecewise_construct,
                        std::forward_as_tuple(key, key_sz),
                        std::forward_as_tuple(value, value_sz));
  return 0;
}

extern "C"
int
fox_get(fox_ptr fhand,
        const char *key, size_t key_sz,
        char **pvalue, size_t *pvalue_sz)
{
  auto it = fhand->table.find(std::string(key, key_sz));
  if (it == fhand->table.end()) {
    return -1;
  }

  char* buf = static_cast<char*>(malloc(it->second.length()));
  std::strncpy(buf, it->second.c_str(), it->second.length());
  *pvalue_sz = it->second.length();
  *pvalue = buf;

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
  //FIXME: no semaphore stuff
  return fox_set(fhand, key, key_sz, value, value_sz);
}

extern "C"
int
fox_sync_get(fox_ptr fhand, unsigned delta,
             const char *key, size_t key_sz,
             char **pvalue, size_t *pvalue_sz)
{
  //FIXME: no semaphore stuff
  return fox_get(fhand, key, key_sz, pvalue, pvalue_sz);
}

extern "C"
int
fox_broad_set(fox_ptr fhand,
              const char *key, size_t key_sz,
              const char *value, size_t value_sz)
{
  //FIXME: no broadcast stuff
  return fox_set(fhand, key, key_sz, value, value_sz);
}

extern "C"
int
fox_broad_get(fox_ptr fhand,
              const char *key, size_t key_sz,
              char **pvalue, size_t *pvalue_sz)
{
  //FIXME: no broadcast stuff
  return fox_get(fhand, key, key_sz, pvalue, pvalue_sz);
}

extern "C"
int
fox_queue_set(fox_ptr fhand,
              const char *key, size_t key_sz,
              const char *value, size_t value_sz)
{
  fhand->queue.emplace(std::piecewise_construct,
                       std::forward_as_tuple(key, key_sz),
                       std::forward_as_tuple(value, value_sz));
  return 0;
}

extern "C"
int
fox_queue_get(fox_ptr fhand,
              const char *key, size_t key_sz,
              char **pvalue, size_t *pvalue_sz)
{
  auto it = fhand->queue.find(std::string(key, key_sz));
  if (it == fhand->queue.end()) {
    return -1;
  }

  char* buf = static_cast<char*>(malloc(it->second.length()));
  std::strncpy(buf, it->second.c_str(), it->second.length());
  *pvalue = buf;
  *pvalue_sz = it->second.length();

  fhand->queue.erase(it);

  return 0;
}

extern "C"
int
fox_broad_queue_set(fox_ptr fhand,
                    const char *key, size_t key_sz,
                    const char *value, size_t value_sz)
{
  return fox_queue_set(fhand, key, key_sz, value, value_sz);
}

extern "C"
int
fox_dist_queue_set(fox_ptr fhand,
                   const char *key, size_t key_sz,
                   const char *value, size_t value_sz)
{
  return fox_queue_set(fhand, key, key_sz, value, value_sz);
}

extern "C"
int
fox_dist_queue_get(fox_ptr fhand,
                   const char *key, size_t key_sz,
                   char **pvalue, size_t *pvalue_sz)
{
  return fox_queue_get(fhand, key, key_sz, pvalue, pvalue_sz);
}

extern "C"
int
fox_reduce_set(fox_ptr fhand,
               const char *key, size_t key_sz,
               const char *value, size_t value_sz)
{
  return fox_set(fhand, key, key_sz, value, value_sz);
}

extern "C"
int
fox_reduce_get(fox_ptr fhand,
               const char *key, size_t key_sz,
               char *pvalue, size_t pvalue_sz,
               void (*reduce)(void *out, void *in))
{
  char* tmpvalue;
  size_t tmpvalue_sz;
  int ret = fox_get(fhand, key, key_sz, &tmpvalue, &tmpvalue_sz);
  if (ret != 0) {
    return ret;
  }

  (*reduce)(pvalue, tmpvalue);
  free(tmpvalue);

  return 0;
}

extern "C"
int
fox_collect(fox_ptr fhand,
            const char *key, size_t key_sz,
            int root, int opt)
{
  return 0;
}
