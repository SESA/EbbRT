EbbRT cmdline library
===

## Requirements

* (Optional) Set the `EBBRT_SRCDIR` environment variable to point to 
EbbRT source directory `export EBBRT_SRCDIR=~/EbbRT`
* Build and install EbbRT native toolchain, assume installed at `~/sysroot/native`
* Build and install EbbRT hosted library, assume installed at `~/sysroot/hosted`

## Build and install for hosted

```
  mkdir hosted
  cd hosted
  cmake -DCMAKE_INSTALL_PREFIX=~/sysroot/hosted -DCMAKE_PREFIX_PATH=~/sysroot/hosted ..
  make -j install
 ```
 
## Build and install for native

```
  mkdir native
  cd native
  cmake -DCMAKE_TOOLCHAIN_FILE=~/sysroot/native/usr/misc/ebbrt.cmake ..
  make -j install
 ```
