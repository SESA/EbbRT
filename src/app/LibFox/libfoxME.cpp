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
#include <cassert>

extern "C" {
#include "src/app/LibFox/libfox.h"
}

#include "src/ebb/ebb.hpp"
#include "ebbs.hpp"

#define TRACE printf("%s:%d:%s\n", __FILE__, __LINE__,__func__)

struct fox_st {
  ebbrt::EbbRT instance;
  ebbrt::Context context{instance};
};

extern "C"
int
fox_new(fox_ptr* fhand_ptr, int nprocs, int procid)
{
  struct fox_st *fh = new fox_st;

  *fhand_ptr = fh;
  // fix me, do a constructor/destructor around activate/deactivate
  fh->context.Activate();  

  return 0;
}

extern "C"
int
fox_free(fox_ptr fhand)
{
  TRACE;
  return 0;
}

extern "C"
int
fox_flush(fox_ptr fhand, int term)
{
  TRACE;
  return 0;
}

extern "C"
int
fox_server_add(fox_ptr fhand, const char *hostlist)
{
  TRACE;
  return 0;
}

extern "C"
int
fox_set(fox_ptr fhand,
        const char *key, size_t key_sz,
        const char *value, size_t value_sz)
{
  TRACE;
  ebbrt::EbbRef<ebbrt::fox::Object> o= ebbrt::fox::theHash->get(key);

  if (o != ebbrt::EbbRef<ebbrt::fox::Object>(ebbrt::NULLID)) assert(0); // should not exist yet
  
  o = static_cast<ebbrt::EbbRef<ebbrt::fox::Object>>(ebbrt::fox::RDData::Create());
  // JA NOTE: THIS IS NATURAL C++ BUT DANGEROUS CO code as  value() is a deref and could fail
  // I guess in our model this should be an exception so this should be a try block?
  o->value().set(value, value_sz);
  ebbrt::fox::theHash->set(key, o);
  return 0;
}

extern "C"
int
fox_get(fox_ptr fhand,
        const char *key, size_t key_sz,
        char **pvalue, size_t *pvalue_sz)
{
  TRACE;
  ebbrt::EbbRef<ebbrt::fox::Object> o= ebbrt::fox::theHash->get(key);
  if (o != ebbrt::EbbRef<ebbrt::fox::Object>(ebbrt::NULLID)) {
    o->value().get(pvalue, pvalue_sz);
  } else {
    *pvalue = NULL;
    *pvalue_sz = 0;
  }
  return 0;
}

extern "C"
int
fox_delete(fox_ptr fhand, const char* key, size_t key_sz)
{
  TRACE;
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
  TRACE;
  assert(strcmp(key,STR_TASK_SYNC)==0);
  assert(ebbrt::fox::theTaskSync == static_cast<ebbrt::EbbRef<ebbrt::fox::Sync>>(ebbrt::fox::theHash->get(key)));
  ebbrt::fox::theTaskSync->enter((void *)__func__);
  return 0;
}

extern "C"
int
fox_sync_get(fox_ptr fhand, unsigned delta,
             const char *key, size_t key_sz,
             char **pvalue, size_t *pvalue_sz)
{
  //FIXME: no semaphore stuff
  TRACE;
  assert(strcmp(key,STR_TASK_SYNC)==0);
  assert(ebbrt::fox::theTaskSync == static_cast<ebbrt::EbbRef<ebbrt::fox::Sync>>(ebbrt::fox::theHash->get(key)));
  ebbrt::fox::theTaskSync->enter((void *)__func__);
  return 0;
}

extern "C"
int
fox_broad_set(fox_ptr fhand,
              const char *key, size_t key_sz,
              const char *value, size_t value_sz)
{
  //FIXME: no broadcast stuff
  ebbrt::EbbRef<ebbrt::fox::Dictionary> dic = ebbrt::fox::theHash;
  ebbrt::EbbRef<ebbrt::fox::GatherData> rw = ebbrt::fox::theRWData;
  TRACE;
  assert(dic != rw);
  assert(strcmp(key,STR_RDDATA)==0);
  ebbrt::fox::theHash->get(key);
  assert(ebbrt::fox::theRDData == static_cast<ebbrt::EbbRef<ebbrt::fox::RDData>>(ebbrt::fox::theHash->get(key)));
  ebbrt::fox::theRDData->set(value, value_sz);
  return 0;
}

extern "C"
int
fox_broad_get(fox_ptr fhand,
              const char *key, size_t key_sz,
              char **pvalue, size_t *pvalue_sz)
{
  //FIXME: no broadcast stuff
  TRACE;
  assert(strcmp(key,STR_RDDATA)==0);
  *pvalue = (char *)ebbrt::fox::theRDData->get(pvalue_sz);
  return 0;
}

extern "C"
int
fox_queue_set(fox_ptr fhand,
              const char *key, size_t key_sz,
              const char *value, size_t value_sz)
{
  TRACE;
  assert(strcmp(key,STR_TASKS)==0);
  ebbrt::fox::theTaskQ->enque(value, value_sz);
  return 0;
}

extern "C"
int
fox_queue_get(fox_ptr fhand,
              const char *key, size_t key_sz,
              char **pvalue, size_t *pvalue_sz)
{
  TRACE;
  assert(strcmp(key,STR_TASKS)==0);
  *pvalue = (char *)ebbrt::fox::theTaskQ->deque(pvalue_sz);
  return 0;
}

extern "C"
// round robins to different queues
int
fox_broad_queue_set(fox_ptr fhand,
                    const char *key, size_t key_sz,
                    const char *value, size_t value_sz)
{
  TRACE;
  assert(strcmp(key,STR_TASKS)==0);
  ebbrt::fox::theTaskQ->enque(value, value_sz);
  return 0;
}

// adds to all 
extern "C"
int
fox_dist_queue_set(fox_ptr fhand,
                   const char *key, size_t key_sz,
                   const char *value, size_t value_sz)
{
  TRACE;
  assert(strcmp(key,STR_TASKS)==0);
  ebbrt::fox::theTaskQ->enque(value, value_sz);
  return 0;
}

extern "C"
int
fox_dist_queue_get(fox_ptr fhand,
                   const char *key, size_t key_sz,
                   char **pvalue, size_t *pvalue_sz)
{
  TRACE;
  assert(strcmp(key,STR_TASKS)==0);
  *pvalue = (char *)ebbrt::fox::theTaskQ->deque(pvalue_sz);
  return 0;
}

extern "C"
int
fox_reduce_set(fox_ptr fhand,
               const char *key, size_t key_sz,
               const char *value, size_t value_sz)
{
  TRACE;
  assert(strcmp(key,STR_RWDATA)==0);
  ebbrt::fox::theRWData->add(value, value_sz);
  return 0;
}

extern "C"
int
fox_reduce_get(fox_ptr fhand,
               const char *key, size_t key_sz,
               char *pvalue, size_t pvalue_sz,
               void (*reduce)(void *out, void *in))
{
  TRACE;
  void *data;
  size_t len;

  assert(strcmp(key,STR_RWDATA)==0);
  data = ebbrt::fox::theRWData->gather(&len);
  (*reduce)(pvalue, data);
  return 0;
}

extern "C"
int
fox_collect(fox_ptr fhand,
            const char *key, size_t key_sz,
            int root, int opt)
{
  TRACE;
  assert(0);
  return 0;
}
