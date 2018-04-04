=======
# Build and Install the EbbRT Native/Hosted Libraries
#
# examples: 
# 	$ make -f ~/EbbRT/Makefile -j=12 VERBOSE=1
#
# Options:
# 	DEBUG=1 						# build without optimisation
# 	CLEANUP=1     			# remove build state when finished
# 	PREFIX=<path> 			# install directory [=/usr/local] 
# 	BUILD_ROOT					# build directory [=$PWD]
# 	EBBRT_SRCDIR=<path> # EbbRT source repository 
# 	VERBOSE=1   				# verbose build 

-include config.mk # Local config (optional)

CD ?= cd
CMAKE ?= cmake
MAKE ?= time make
MKDIR ?= mkdir
RM ?= -rm

PREFIX ?= /usr/local 				# Install Linux libraries to /usr/local
EBBRT_SYSROOT ?= /opt/ebbrt # Install Native libraries to /opt/ebbrt
BUILD_ROOT ?= $(abspath $(CURDIR))
HOSTED_BUILD_DIR ?= $(BUILD_ROOT)/hosted_build
NATIVE_BUILD_DIR ?= $(BUILD_ROOT)/native_build
EBBRT_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
NATIVE_TOOLCHAIN_FILE ?= $(EBBRT_SRCDIR)/src/cmake/ebbrt.cmake

# Assume this Makefile is located in the root directory of EbbRT repo 
ifeq "$(wildcard $(EBBRT_SRCDIR) )" ""
  $(error Unable to locate source EBBRT_SRCDIR=$(EBBRT_SRCDIR))
endif

ifdef CLEANUP
CLEANUP ?= $(RM) -rf  
else
CLEANUP ?= test 
endif

ifdef DEBUG
CMAKE_BUILD_TYPE ?= -DCMAKE_BUILD_TYPE=Debug
MAKE_BUILD_OPT ?= DEBUG=1
else
CMAKE_BUILD_TYPE ?= -DCMAKE_BUILD_TYPE=Release
MAKE_BUILD_OPT ?= 
endif

ifdef VERBOSE
MAKE_VERBOSE_OPT ?= VERBOSE=1
CMAKE_VERBOSE_OPT ?= -DCMAKE_VERBOSE_MAKEFILE=On
else
MAKE_VERBOSE_OPT ?= 
CMAKE_VERBOSE_OPT ?= 
endif

# Assume this file is located in the root directory of EbbRT repo 
ifeq "$(wildcard $(EBBRT_SRCDIR) )" ""
  $(error Unable to locate source EBBRT_SRCDIR=$(EBBRT_SRCDIR))
endif
EBBRT_SRC ?= $(EBBRT_SRCDIR)/src
EBBRT_LIBS ?= $(EBBRT_SRCDIR)/libs
EBBRT_BUILD_DEFS ?= -DCMAKE_C_COMPILER_FORCED=1 \
                    -DCMAKE_CXX_COMPILER_FORCED=1

all: hosted native 

install: hosted-install native-install

ebbrt-libs: native-libs hosted-libs

build: hosted native

install: hosted-install native-install

clean:
	$(MAKE) -C $(HOSTED_BUILD_DIR) clean
	$(MAKE) -C $(NATIVE_BUILD_DIR) clean

hosted: | $(EBBRT_SRC)
	$(MKDIR) -p $(HOSTED_BUILD_DIR) && $(CD) $(HOSTED_BUILD_DIR) && \
	$(CMAKE) -DCMAKE_INSTALL_PREFIX=$(PREFIX)  \
	$(CMAKE_BUILD_TYPE) $(CMAKE_VERBOSE_OPT) $(EBBRT_SRC) 
	$(MAKE) -C $(HOSTED_BUILD_DIR) $(MAKE_OPT) 

hosted-install: | $(HOSTED_BUILD_DIR)
	$(MAKE) -C $(HOSTED_BUILD_DIR) $(MAKE_OPT) install 

hosted-libs: hosted-install 
	$(CD) $(HOSTED_BUILD_DIR) && $(MKDIR) -p libs && $(CD) libs && \
	$(CMAKE) -DCMAKE_INSTALL_PREFIX=$(PREFIX)  \
	-DCMAKE_PREFIX_PATH=$(PREFIX) \
	$(CMAKE_BUILD_TYPE) $(CMAKE_VERBOSE_OPT) $(EBBRT_LIBS) && \
	$(MAKE) $(MAKE_OPT) install && $(CD) - 

native: | $(EBBRT_SRC) $(NATIVE_TOOLCHAIN_FILE)
	$(MKDIR) -p $(NATIVE_BUILD_DIR) && $(CD) $(NATIVE_BUILD_DIR) && \
	EBBRT_SYSROOT=$(EBBRT_SYSROOT) $(CMAKE) \
	-DCMAKE_INSTALL_PREFIX:PATH=$(SYSROOT_PREFIX) \
	$(EBBRT_BUILD_DEFS) \
	-DCMAKE_TOOLCHAIN_FILE=$(NATIVE_TOOLCHAIN_FILE) \
	$(CMAKE_BUILD_TYPE) $(CMAKE_VERBOSE_OPT) $(EBBRT_SRC) 
	$(MAKE) -C $(NATIVE_BUILD_DIR) $(MAKE_OPT) 

native-install: | $(NATIVE_BUILD_DIR)
	$(MAKE) -C $(NATIVE_BUILD_DIR) $(MAKE_OPT) install 

native-libs: native-install
	$(CD) $(NATIVE_BUILD_DIR) && $(MKDIR) -p libs && $(CD) libs && \
	EBBRT_SYSROOT=$(EBBRT_SYSROOT) $(CMAKE) \
	-DCMAKE_INSTALL_PREFIX:PATH=$(SYSROOT_PREFIX) \
	$(EBBRT_BUILD_DEFS) \
	-DCMAKE_TOOLCHAIN_FILE=$(NATIVE_TOOLCHAIN_FILE) \
	$(CMAKE_BUILD_TYPE) $(CMAKE_VERBOSE_OPT) $(EBBRT_LIBS) && \
	$(MAKE) $(MAKE_OPT) install && $(CD) - 

.PHONY: all clean build install hosted hosted-install native \
				native-install ebbrt-hosted-libs ebbrt-native-libs
