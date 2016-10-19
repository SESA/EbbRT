# Debugging the Native runtime
To debug the native runtime you must have built your application, and all libraries on which your application depends on, unoptimised and with debugging symbols. In general, this is done by passing `-g -O0` to gcc, or by setting `CMAKE_BUILD_TYPE=Debug` when using `cmake`.

####Step 1. 
The `NodeAllocator`(hosted) that has been built for Debug will spawn a native backend which can be connected to with an unmodified **gdb**. 
   
   1.1 If you are booting a `kvm-qmeu` backend VM by hand, use the '-gdb' or `-a` flags to enable gdb connections

####Step 2. 
When a new node is allocated, the `NodeAllocator` will give its local and remote gdb addresses.
```
Node Allocation Details: 
| gdb: 172.19.0.2:1234
| gdb(local): localhost:34042
| container: 227e3f58fade
```

####Step 3. 
Open the native `.elf` file in gdb (*note: not the .elf32*)
```
$ gdb app.elf
```

####Step 4. 
Within gdb, connect to one of the remote gdb addresses. It may take a few seconds to connect...
```
(gdb) target remote localhost:34042
```

####Step 5. 
Once connected, the native runtime supports normal gdb commands and functionality. Please note, the native runtime will have already begun execution before you've connected to it. Good luck!
