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
#
-include config.mk # Local config (optional)

## EXAMPLE DELETE
# Special build flags. Use them like this: "make library=shared"
#ifeq ($(library), shared)
#  GYPFLAGS += -Dcomponent=shared_library
#endif
#ifeq ($(libraries), on) #	# build ebbrt libraries
#  #GYPFLAGS += -Dcomponent=shared_library
#endif
## debugsymbols=on
#ifeq ($(debugsymbols), on)
##  GYPFLAGS += -Drelease_extra_cflags=-ggdb3
#endif

CD ?= cd
CMAKE ?= cmake
MAKE ?= time make
MKDIR ?= mkdir
RM ?= -rm
TEST ?= test
BUILD_ROOT ?= $(abspath $(CURDIR))

EBBRT_SRCDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
NATIVE_TOOLCHAIN_FILE ?= $(EBBRT_SRCDIR)/src/cmake/ebbrt.cmake

# Default install to global path: /usr/local
PREFIX ?= /usr/local
INSTALL_PREFIX ?= $(PREFIX)
NATIVE_SYSROOT_PREFIX ?= $(INSTALL_PREFIX)/ebbrt

# Default build to local path: $PWD 
HOSTED_BUILD_DIR ?= $(BUILD_ROOT)/hosted_build
NATIVE_BUILD_DIR ?= $(BUILD_ROOT)/native_build

ifdef CLEANUP
CLEANUP ?= $(RM) -rf  
else
CLEANUP ?= $(TEST)
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
EBBRT_TOOLCHAIN_MAKEFILE ?= $(EBBRT_SRCDIR)/toolchain/Makefile
EBBRT_BUILD_DEFS ?= -DCMAKE_C_COMPILER_FORCED=1 \
                    -DCMAKE_CXX_COMPILER_FORCED=1

all: hosted native ebbrt-libs

build: hosted native

install: hosted-install native-install

clean:
	$(MAKE) -C $(HOSTED_BUILD_DIR) clean
	$(MAKE) -C $(NATIVE_BUILD_DIR) clean

hosted: | $(EBBRT_SRC)
	$(MKDIR) -p $(HOSTED_BUILD_DIR) && $(CD) $(HOSTED_BUILD_DIR) && \
	$(CMAKE) -DCMAKE_INSTALL_PREFIX=$(INSTALL_PREFIX)  \
	$(CMAKE_BUILD_TYPE) $(CMAKE_VERBOSE_OPT) $(EBBRT_SRC) 
	$(MAKE) -C $(HOSTED_BUILD_DIR) $(MAKE_OPT) 

hosted-install: | $(HOSTED_BUILD_DIR)
	$(MAKE) -C $(HOSTED_BUILD_DIR) $(MAKE_OPT) install 

native: | $(EBBRT_SRC) $(NATIVE_TOOLCHAIN_FILE)
	$(MKDIR) -p $(NATIVE_BUILD_DIR) && $(CD) $(NATIVE_BUILD_DIR) && \
	$(CMAKE) -DCMAKE_INSTALL_PREFIX:PATH=$(NATIVE_SYSROOT_PREFIX) \
	$(EBBRT_BUILD_DEFS) \
	-DCMAKE_TOOLCHAIN_FILE=$(NATIVE_TOOLCHAIN_FILE) \
	$(CMAKE_BUILD_TYPE) $(CMAKE_VERBOSE_OPT) $(EBBRT_SRC) 
	$(MAKE) -C $(NATIVE_BUILD_DIR) $(MAKE_OPT) 

native-install: | $(NATIVE_BUILD_DIR)
	$(MAKE) -C $(NATIVE_BUILD_DIR) $(MAKE_OPT) install 

# TODO these..

ebbrt-libs: ebbrt-native-libs ebbrt-hosted-libs

ebbrt-hosted-libs: hosted 
	$(CD) $(HOSTED_BUILD_DIR) && $(MKDIR) -p libs && $(CD) libs && \
	$(CMAKE) -DCMAKE_INSTALL_PREFIX=$(INSTALL_PREFIX)  \
	$(CMAKE_BUILD_TYPE) $(CMAKE_VERBOSE_OPT) $(EBBRT_LIBS) && \
	$(MAKE) $(MAKE_OPT) install && $(CD) - 
	
ebbrt-native-libs: ebbrt-only 
	$(CD) $(NATIVE_BUILD_DIR) && $(MKDIR) -p libs && $(CD) libs && \
	EBBRT_SYSROOT=$(NATIVE) $(CMAKE) -DCMAKE_INSTALL_PREFIX=$(NATIVE_SYSROOT_PREFIX) \
	-DCMAKE_TOOLCHAIN_FILE=$(NATIVE_TOOLCHAIN_FILE) \
	$(CMAKE_BUILD_TYPE) $(CMAKE_VERBOSE_OPT) $(EBBRT_LIBS) && \
	$(MAKE) $(MAKE_OPT) install && $(CD) - 

.PHONY: all clean build install hosted hosted-build native native-build 
