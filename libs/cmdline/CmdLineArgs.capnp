@0x9b8d5550476d8341;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("cmdlineargs");

struct GlobalData {
  args @0 :List(Text);
}