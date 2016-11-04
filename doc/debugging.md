# Debugging the EbbRT native runtime with GDB
The EbbRT native runtime provides support for the gdb debugger, which can be
used to connect remotly to a running EbbRT back-end VM.  Debugging on EbbRT supports the normal `gdb` commands and functionality. 

Note, for full debug transparency, building you application unoptimised and with debug symbols is not enough. Your application must also link with Ebb libraries that have been similarly build for debugging. In general, this is done by passing `CMAKE_BUILD_TYPE=Debug` when using `cmake`. See our build tutorial for instructions for building the EbbRT toolchain for debugging. 

####Step 1. 
When a new node is allocated, the `NodeAllocator` will give a local and remote gdb address:
```
Node Allocation Details: 
| gdb: 172.19.0.2:1234
| gdb(local): localhost:34042
| container: 227e3f58fade
```
Note, only the Debug `NodeAllocator` will provide the gdb service on back-ends.

####Step 2. 
Open the native `.elf` file in gdb (*note: not the .elf32*)
```
$ gdb app.elf
```

####Step 4. 
Within gdb, connect to the remote gdb addresses. It may take a few seconds to connect...
```
(gdb) target remote 172.19.0.2:34042
```
Once gdb connect, the native runtime will have already have begun execution. Normal `gdb` commands and functions are supported.  Good luck!
