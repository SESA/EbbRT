# Elastic Building Blocks (Ebbs)
Ebbs are the primary programming model for EbbRT. An Ebb are invoked the same
as a standard C++ object on which you can call methods (e.g., `foo->bar()`),
however, Ebbs are unique in that they are lazily initialized and are designed
to abstract the distributed (or shared) nature of the underlying object. 

## EbbRef
The EbbRef<T> template, templated by a compatible object type, T, creates a
reference to an Ebb, through which the object can be interacted with. By
definition, the minimal requirement of an Ebb compatible object is that it
supports _lazily instantiation_. This is establish through the definition of
an `HandleFault(EbbID id)` method and enforced at compile time (through the
EbbRef template definition).

## EbbId
Each Ebb reference corresponds with an EbbID. EbbIDs are not necessarily
unique as they maybe part of a local or global namespace. 

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
Default Ebb functionality are provided through templates. 
* [SharedEbb](../blob/master/src/SharedEbb.h)
  A Ebb with a per-node instance that is shared across all cores
* [MulticoreEbb](../blob/master/src/MulticoreEbb.h)
  An Ebb which will construct per-core representatives 
* [StaticSharedEbb](../blob/master/src/StaticSharedEbb.h)
  A SharedEbb that is constructed with a static ID. 
* [MulticoreEbbStatic](../blob/master/src/MulticoreEbbStatic.h)
  A Multicore Ebb that is constructed with a static ID. 

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


## Overview  of Ebb Translation System
The translation system is one of the core responsibilities of the low-level
EbbRT functionality. The job of translation is to
convert an generic Ebb object (an ID) into the execution of a method of the
local representative (rep) of that Ebb. The common path of translation
involves a lookup in a local translation cache that contains references to the
local ebb representative of a particular ebb id.

A translation cache miss will check a global translation table for needed
reference. Upon a hit in the global table, the reference and id will be added
to that cores local translation table. Depending on the Ebb type of the object
being referenced, or the current state of the ebb, a new representation Ebb
maybe allocated during the miss process. 
<!-- If a reference is
 not found in the node-local view of the global address space, a global miss,
 the Ebb Manager ebb will handles a remote lookup for the necessary
 information. -->

Local ebb allocations happen in a lazy fashion, following misses in the
translation table lookups. This lazy initiation is allowed for all system
ebbs, including those most primitive (EbbManager, MemoryManager,etc.) to be created only when they are first needed. 
To bootstrap this process, we statically construct a number of Ebb roots on boot.
An initial global translation table (aka. root table) is populated with the
Ebb roots necessary to construct the most primitive of ebb objects. This will
prevent a global miss before the system is fully initialized. 
<!--The root table
is fixed and will eventually need to be replaced by a more dynamic/distributed
global translation functionality provided by the Ebb Manager. -->
