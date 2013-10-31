#include "app/app.hpp"
#include "src/ebb/ebb.hpp"
#include "ebbs.hpp"
#include "ebb/SharedRoot.hpp"
#include <cstring>
#include <iostream>

#define TRACE   printf("> %s:%d:%s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__)


// registers symbol for configuration
__attribute__((constructor(65535)))
static void _reg_symbol()
{
  ebbrt::app::AddSymbol (STR_RDDATA, ebbrt::fox::RDData::ConstructRoot);
  ebbrt::app::AddSymbol (STR_TASKS, ebbrt::fox::TaskQ::ConstructRoot);
  ebbrt::app::AddSymbol (STR_TASK_SYNC, ebbrt::fox::TaskSync::ConstructRoot);
  ebbrt::app::AddSymbol (STR_RWDATA, ebbrt::fox::RWData::ConstructRoot);
  ebbrt::app::AddSymbol (STR_HASH, ebbrt::fox::Hash::ConstructRoot);
}

ebbrt::EbbRoot*
ebbrt::fox::Hash::ConstructRoot()
{
  TRACE;
  return new SharedRoot<Hash>;
}

ebbrt::fox::Hash::Hash()
{
  //FIXME:  Not really sure I like public constructors when then 
  //        reps really should only be constructed by the root

  // for completelness we put ourselves in the dictionary
  TRACE;

  set(STR_HASH, static_cast<EbbRef<Object>>(theHash));

  // put the rest of the well known instances in the dictionary
  set(STR_RDDATA, static_cast<EbbRef<Object>>(theRDData));
  set(STR_TASKS, static_cast<EbbRef<Object>>(theTaskQ));
  set(STR_TASK_SYNC, static_cast<EbbRef<Object>>(theTaskSync));
  set(STR_RWDATA, static_cast<EbbRef<Object>>(theRWData)); 

#if 0
  std::cout << "Hash::Hash(): map contains:";
  for ( auto it = map.begin(); it != map.end(); ++it )
    std::cout << " " << it->first << ":" << it->second;
  std::cout << std::endl;
#endif
}

void 
ebbrt::fox::Hash::set(const char *key, EbbRef<ebbrt::fox::Object> o)
{
  map[key] = o;
  TRACE;
}

ebbrt::lrt::trans::EbbRef<ebbrt::fox::Object>
ebbrt::fox::Hash::get(const char * key)
{
  TRACE;
#if 0
  printf("Hash::get(): rep=%p\n", this);
  std::cout << "Hash::get(): map contains:";
  for ( auto it = map.begin(); it != map.end(); ++it )
    std::cout << " " << it->first << ":" << it->second;
  std::cout << std::endl;
#endif

  std::unordered_map<std::string,EbbRef<Object>>::const_iterator got = map.find(key);
  if ( got == map.end() )
    return EbbRef<Object>(NULLID);
  else
    return got->second;
}

ebbrt::EbbRoot*
ebbrt::fox::RDData::ConstructRoot()
{
  TRACE;
  return new SharedRoot<RDData>;
}

ebbrt::EbbRef<ebbrt::fox::RDData>
ebbrt::fox::RDData::Create()
{
  EbbRef<RDData> ref = EbbRef<RDData>(ebb_manager->AllocateId());
  ebb_manager->Bind(ConstructRoot, ref);
  return ref;
}

void
ebbrt::fox::RDData::set(const void * data, size_t len) 
{ 
  assert(val_.unset());
  char * b = new char[len];
  memcpy(b, data, len);
  val_.set(b, len);
}


void *
ebbrt::fox::RDData::get(size_t *len)
{ 
  char * b;
  val_.get(&b, len);
  return b;
}


ebbrt::EbbRoot*
ebbrt::fox::TaskQ::ConstructRoot()
{
  TRACE;
  return new SharedRoot<TaskQ>;
}

int
ebbrt::fox::TaskQ::enque(const void * data, size_t len) 
{ 
  char *p;
  p = new char[len];
  memcpy(p, data, len);
  myqueue.emplace(p, len);
  return 0;
}

void *
ebbrt::fox::TaskQ::deque(size_t *len) 
{
  void *data;
  struct el &hd = myqueue.front();
  // note, data allocated at start, returns pointer freed by 
  // callee
  data = hd.ptr;
  *len = hd.len;
  myqueue.pop();
  return data; 
}


ebbrt::EbbRoot*
ebbrt::fox::TaskSync::ConstructRoot()
{
  TRACE;
  return new SharedRoot<TaskSync>;
}

void
ebbrt::fox::TaskSync::enter(const void * data) 
{ 
  TRACE;
}

ebbrt::EbbRoot*
ebbrt::fox::RWData::ConstructRoot()
{
  TRACE;
  return new SharedRoot<RWData>;
}

void
ebbrt::fox::RWData::add(const void * data, size_t len)
{
  TRACE;
  char *p;
  p = new char[len];
  memcpy(p, data, len);
  myqueue.emplace(p, len);
  return ;
}

// FIXME: this should return all the data from all the adds
void *
ebbrt::fox::RWData::gather(size_t *len)
{
  struct el &hd = myqueue.front();
  void *data;

  // TRACE;
  // note, data allocated at start, returns pointer freed by 
  // callee
  data = hd.ptr;
  *len = hd.len;
  myqueue.pop();
  return data; 
}


