@0xab1ac5096afdee55;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("ebbrt::global_id_map_message");

struct GetRequest {
  messageId @0 :UInt64;
  ebbId @1 :UInt32;
}

struct SetRequest {
  messageId @0 :UInt64;
  ebbId @1 :UInt32;
  data @2 :Data;
}

struct Request {
  union {
    getRequest @0 :GetRequest;
    setRequest @1 :SetRequest;
  }
}

struct GetReply {
  messageId @0 :UInt64;
  data @1 :Data;
}

struct SetReply {
  messageId @0 :UInt64;
}

struct Reply {
  union {
    getReply @0 :GetReply;
    setReply @1 :SetReply;
  }       
}