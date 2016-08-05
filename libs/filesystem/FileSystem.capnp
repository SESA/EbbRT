@0xa11e7074f65137ca;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("filesystem");

struct GlobalData {
  networkAddress @0 :Data;
}

struct StatRequest {
  id @0 :UInt32;
  file :union {
    path @1 :Text;
    lpath @2 :Text;
    fd @3 :Int32;
  }
}

struct StatReply {
  id @0 :UInt32;
  dev @1 :UInt64;
  ino @2 :UInt64;
  mode @3 :UInt64;
  nlink @4 :UInt64;
  uid @5 :UInt64;
  gid @6 :UInt64;
  rdev @7 :UInt64;
  size @8 :UInt64;
  atime @9 :UInt64;
  mtime @10 :UInt64;
  ctime @11 :UInt64;       
}

struct GetCwdRequest {
  id @0 :UInt32;
}

struct GetCwdReply {
  id @0 :UInt32;
  cwd @1 :Text;
}

struct OpenRequest {
  id @0 :UInt32;
  flags @1 :Int32;
  mode @2 :Int32;
  path @3 :Text;
}

struct OpenReply {
  id @0 :UInt32;
  fd @1 :UInt32;
}

struct ReadRequest {
  id @0 :UInt32;
  fd @1 :UInt32;
  length @2 :UInt64;
  offset @3 :Int64;
}

struct ReadReply {
  id @0 :UInt32;
  data @1 :Data;      
}

struct Request {
  union {
    statRequest @0 :StatRequest;
    getCwdRequest @1 :GetCwdRequest;
    openRequest @2 :OpenRequest;
    readRequest @3 :ReadRequest;
  }
}

struct Reply {
  union {
    statReply @0 :StatReply;
    getCwdReply @1 :GetCwdReply;
    openReply @2 :OpenReply;
    readReply @3 :ReadReply;
  }
}

struct Message {
  union {
    request @0 :Request;
    reply @1 :Reply;
  }       
}