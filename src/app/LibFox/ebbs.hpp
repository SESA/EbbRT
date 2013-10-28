#ifndef __FOX_EBBS_H__
#define __FOX_EBBS_H__


#define STR_RDDATA    "RD"
#define STR_RWDATA    "WR"
#define STR_TASKS     "TK"
#define STR_TASK_SYNC "TKS"
#define STR_HASH      "HASH"

#include <string>
#include <unordered_map>
#include <queue>


namespace ebbrt {
  // JAHACK: not sure if we have one but 
  // adding here has a hack till we
  // talk about it
  const EbbId NULLID = 0;
  
  namespace fox {
    class RDData;

    class  Value {
      void * bytes_;
      size_t len_;
    public:
      Value() : bytes_(NULL), len_(0) {}
      void get(char **b, size_t *l) { *b = (char *)bytes_; *l = len_; }
      void set(const char *b, size_t l) { bytes_ = (char *)b; len_ = l; }
      int unset() { return (bytes_ == 0 && len_ == 0); }
    };

    class Object : public EbbRep {
    protected:
      Value val_;
    public:
      //      Object(const char *str);
      virtual void getValue(char **b, size_t *s) { val_.get(b, s); }
      virtual void setValue(const char *b, size_t s) { val_.set(b, s); }
    };

    class Dictionary : public Object {
    public:
      virtual void get(const char *key, EbbRef<Object> *obj) = 0;
      virtual void set(const char *key, EbbRef<Object> obj) = 0;
    };

    class ScatterData : public Object {
    public:
      virtual void set(const void * data, size_t len) = 0;
      virtual void *get(size_t *len) = 0;
    };

    // push/pop queue
    class Queue : public Object {
    public:
      virtual int enque(const void * data, size_t len) = 0;
      virtual void *deque(size_t *len) = 0;
    };

    // FIXME:barrier, 
    // does a sync_get for number of processors
    // sets don't block...
    class Sync : public Object {
    public:
      virtual void enter(const void * data) = 0;
    };

    class GatherData : public Object {
    public:
      virtual void add(const void * data, size_t len)=0;
      virtual void *gather(size_t *len)=0;
    };

    const EbbRef<ScatterData> theRDData =
      EbbRef<ScatterData>(lrt::config::find_static_ebb_id(STR_RDDATA));

    const EbbRef<Queue> theTaskQ =
      EbbRef<Queue>(lrt::config::find_static_ebb_id(STR_TASKS));

    const EbbRef<Sync> theTaskSync =
      EbbRef<Sync>(lrt::config::find_static_ebb_id(STR_TASK_SYNC));

    const EbbRef<GatherData> theRWData =
      EbbRef<GatherData>(lrt::config::find_static_ebb_id(STR_RWDATA));

    const EbbRef<Dictionary> theHash = 
      EbbRef<Dictionary>(lrt::config::find_static_ebb_id(STR_HASH));
  }
}

namespace ebbrt {
  namespace fox {
    class Hash : public Dictionary {
      std::unordered_map<std::string, EbbRef<Object>> map;
    public:
      static EbbRoot * ConstructRoot();
      Hash();
      virtual void get(const char *key, EbbRef<Object> *obj) override;
      virtual void set(const char *key, EbbRef<Object> obj) override;
    };

    class RDData : public ScatterData {
#if 0
      void *buf;
      size_t buf_len;
#endif
    public:
      static EbbRoot * ConstructRoot();
      static EbbRef<RDData> Create();
      virtual void set(const void * data, size_t len) override;
      virtual void *get(size_t *len) override;
    };

    class TaskQ : public Queue  {
      struct el {
	char *ptr;
	size_t len;
	
	el(char *p, size_t l):ptr(p),len(l){}
      };
      std::queue<el> myqueue;

    public:
      static EbbRoot * ConstructRoot();
      virtual int enque(const void * data, size_t len) override;
      virtual void * deque(size_t *len) override;
    };

    class TaskSync : public Sync  {
    public:
      static EbbRoot * ConstructRoot();
      virtual void enter(const void * data) override;
    };

    class RWData : public GatherData {
      // for now, bullshit, returns back one element, the gather should return 
      // all the data, do we want to do it by returning stuff until nothing left
      // also, we need to know how many nodes will put data in to get this right. 
      struct el {
	char *ptr;
	size_t len;
	
	el(char *p, size_t l):ptr(p),len(l){}
      };
      std::queue<el> myqueue;
    public:
      static EbbRoot * ConstructRoot();
      virtual void add(const void * data, size_t len) override;
      virtual void *gather(size_t *len) override;
    }; 
  }
}


#endif
