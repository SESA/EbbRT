This tutorial will guide you through the steps of building and deploying the `hello-world` EbbRT application. The application exists in two parts: 1) a standard Linux application that links in the __hosted__ (Linux-compatible) EbbRT library, and 2) a bootable __native__ `elf` image that built using our toolchain and can be run within a `kvm-qemu` virtual machine environment. 

This tutorial consists of four parts:
1. Build the EbbRT toolchain and __native__ EbbRT libraries. 
2. Build the __hosted__ EbbRT library for Linux.
3. Build the front and back-end of the `hello-world` application. 
4. Deploy EbbRT hello-world with `docker`

We primarily use **make** and **cmake** to build the EbbRT libraries and applications.

### System Requirements
* >= capnproto 0.4.0 
* >= cmake 2.8.12
* >= docker 1.12.0
* >= gcc 5.2.0
* >= libboost 1.54.0 (coroutine and filesystem libraries)
* >= tbb 4.2.0 
* >= m4 1.4.17
* >= texinfo 5.2.0

Install all the required packages (except docker, see below) on Ubuntu 16.04 with the following command:
```
$ apt-get build-essential m4 texinfo cmake libboost-coroutine-dev libboost-dev libboost-filesystem-dev libtbb-dev capnproto libcapnp-dev
```
Instruction for installing docker on Ubuntu 16.04 are available here: https://docs.docker.com/engine/installation/linux/ubuntulinux/

# Step 1. Build and install the EbbRT toolchain
This step will build the complete toolchain and __native__ EbbRT library. Once build, the binaries and headers are installed to a user-defined `sysroot` directory. 

Caution, if not run in parallel it's likely to take a very long time to build our customized GNU toolchain and all libraries dependencies. Furthermore, the build and install files will consume a few gigs of disk space. For specifics, see the Build Statistics page in the project wiki: https://github.com/SESA/EbbRT/wiki/Build-Statistics

Tip: With `make` command include the '-j' flag to specify a parallel build. 

The default `make` of the toolchain will build (with optimizations) and install the toolchain within the working directory.
```
$ make -f $EBBRT_SRCDIR/toolchain/Makefile
```

You can also specify the sysroot install directory,
```
$ make -f $EBBRT_SRCDIR/toolchain/Makefile SYSROOT=$NATIVE_INSTALL_DIR 
```
or enable a debug build.
```
$ make -f $EBBRT_SRCDIR/toolchain/Makefile DEBUG=1
```


# Step 2. Build and install EbbRT hosted library 
We build the Linux library using the `CMakeLists.txt` file is located in `$EBBRT_SRCDIR/src`. Use the `CMAKE_INSTALL_PATH` with `cmake` specify a install path.
```
$ cmake -DCMAKE_INSTALL_PREFIX=$HOSTED_INSTALL_DIR $EBBRT_SRCDIR/src
$ make install
```

## Step 3. Build hello-world application
The default `make` command will build both the application's front-end and back-end. To do so, we need to specify the location of the install directories of the sysroot and hosted libraries. 
```
$ cd $EBBRT_SRCDIR/apps/hello-world
$ EBBRT_SYSROOT=$NATIVE_INSTALL_PATH CMAKE_PREFIX_PATH=$HOSTED_INSTALL_PATH make
```
That's it. Everything is build. You can now jump to **Step 4**, or read on to see how to build the individual front-end and back-end binaries.


A couple gotchas: 
* Front-end `hello-world` assume the back-end is located at `${PWD}/bm/hello-world.elf`. You can change this in the source.
* During build you will see several of the following messages.  THIS IS NORMAL.
```
System is unknown to cmake, create:
Platform/EbbRT to use this system, please send your config file to cmake@www.cmake.org so it can be added to cmake
```

### How to build hello-world.elf
```
$ cmake -DCMAKE_TOOLCHAIN_FILE=$EBBRT_SRCDIR/src/misc/ebbrt.cmake -DCMAKE_PREFIX_PATH=$NATIVE_INSTALL_PATH $EBBRT_SRCDIR/apps/hello-world
$ make
```

### How to build hello-world (Linux) application
```
$ cmake -DCMAKE_PREFIX_PATH=$HOSTED_INSTALL_PATH
$ make
```


# Step 4. Deploy `hello-world`
To deploy the `hello-world` back-end you must have access to `kvm` and the `CAP_NET_ADMIN` capability. This can be done quickly by adding you user to the `kvm` and `docker` groups. Don't forget you must logout + login in to enables the new environment changes. 

```
./hello-world 
```
Running the `hello-world` front-end will make the necessary calls to `docker` to configure and boot the EbbRT back-end. The back-end is booted within a `kvm-qemu` VM that runs within a container. The necessary `Dockerfile` will be downloaded by docker when the container is created, or you can pre-fetch it ahead of time.
```
$ docker pull ebbrt/kvm-qemu:latest
$ docker pull ebbrt/kvm-qemu:debug
```

