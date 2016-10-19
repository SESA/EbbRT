EbbRT filesystem library
===

## Requirements

* (Optional) Set the `EBBRT_SRCDIR` environment variable to point to 
EbbRT source directory `export EBBRT_SRCDIR=~/EbbRT`
* Build and install EbbRT native toolchain, assume installed at `$EBBRT_SRCDIR/toolchain/sysroot`
* Build and install EbbRT hosted library, assume installed at `$EBBRT_SRCDIR/hosted/usr`

## Build and install for hosted

```
  mkdir hosted
  cd hosted
  cmake -DCMAKE_INSTALL_PREFIX=$EBBRT_SRCDIR/hosted/usr -DCMAKE_PREFIX_PATH=$EBBRT_SRCDIR/hosted/us ..
  make -j install
 ```
 
## Build and install for native

```
  mkdir native
  cd native
  cmake -DCMAKE_TOOLCHAIN_FILE=$EBBRT_SRCDIR/toolchain/sysroot/usr/misc/ebbrt.cmake ..
  make -j install
 ```
