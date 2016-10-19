# Elastic Building Blocks (Ebbs)
Ebbs are the primary programming model for EbbRT. An Ebb appears as a standard
C++ object on which you can call methods (e.g., `food->bar()`), however, Ebbs
are unique,  in part, in that they are designed abstract the distributed
nature of the underlying object implementation. 

## EbbRef
The EbbRef<T> template, templated by a compatible object type, T, creates a
reference to an Ebb, through which the object can be interacted with. By
definition, the minimal requirement of an Ebb compatible object is that it
supports _lazily instantiation_. This is establish through the definition of
an `HandleFault(EbbID id)` method and enforced at compile time (through the
EbbRef template definition).

## EbbId
Each Ebb reference corresponds with an EbbID. EbbIDs are not necessarily
unique as they maybe part of a local or global namespace. A small list of
statically define EbbIDs, along with EbbRef template definition are located in
[Trans.h](../blob/master/baremetal/src/include/ebbrt/Trans.h) 

`auto event_manager = EbbRef<EventManager>(kEventManagerId)`

The above line creates a new `EventManager` Ebb reference, `event_manager`,
that is bound to the ID `kEventManagerId`. Although the reference to the
`EventManger` Ebb now exists, the object itself remains uninstantiated. 

## Lazy Instantiation
Ebb objects are allocated and initialized lazily, i.e., only on the first
reference to the object. A local translation table acts as a cache of Ebb
objects (identified by the corresponding EbbIDs) that have been initialized
locally. 

**1) Create an Ebb reference**

`auto fooref = EbbRef<FooObj>(foo_id);`

The `fooref` reference can now be used to make `FooObj` calls.

**2) Make an Ebb call**

`fooref->bar();`

Contained within the EbbRef type is the cache offset, based off the EbbID,
within the local translation cache. Upon dereference, the local cache is
queried for a valid object location.  If the cache is 'hit', the location is
returned and the method is resolved. In the case of a cache 'miss' the
templated object's `HandleFault()` method is called. 

## Ebb Templates
Optional expanded Ebb functionality are provided through templates. 
* [MulticoreEbbStatic](../blob/master/baremetal/src/include/ebbrt/MulticoreEbbStatic.h)
  Defined a multicore (locally distributed) Ebb that is assigned a static
  (unchanging) ID. 

## Ebb Miss
The following steps occur when the first non statically constructed ebb is defined and then called:  
1. Define a new ebb named `enet` of Ebb type `Ethernet`  
`Ebb<Ethernet> enet (ebb_manager->allocateID())`  
2. Bind ebb to an root construction "factory"  
`ebb_manager->bind(virtio::construct_root, enet)`  
3. Make an ebb call  
`enet->AllocateBuffer()`  
4. Call will miss on local translation table and end up in `lrt::_trans_precall()`, which in turn will call the installed miss handler.  
5. The default miss handler will resolve to `InitRoot::PreCall`  
6. The call will again miss, this time on the initial (primative) root table that contains the statically constructed ebbs. This will require a call to `ebb_manager->Install()`  
7. If this is the first install, the ebb_manager will need to **lock**, allocate local data structures, copy over primative root table entries, and construct the root.  
8. Finally, the constructed ebb root of enet will `root->PreCall` which will cache the rep into the local translation table, avoiding further misses. 
