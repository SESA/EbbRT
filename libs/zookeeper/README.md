## ZooKeeper Ebb Library

### Build:

**Hosted:**  
Hosted requires libzookeeper-mt 
```
mkdir -p build/hosted
cmake -DCMAKE_INSTALL_PREFIX={HOSTED_INSTALL_PATH} -DCMAKE_PREFIX_PATH={HOSTED_INSTALL_PATH} ../../
make -j install
```

**Native:**   
Native required EbbRT-socket library to be installed to the sysroot
```
mkdir -p build/hosted
EBBRT_SYSROOT={NATIVE_INSTALL_PATH} cmake -DCMAKE_TOOLCHAIN_FILE={NATIVE_INSTALL_PATH}/usr/misc/ebbrt.cmake 
make -j install
```
