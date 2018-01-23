MYDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

CD ?= cd
CMAKE ?= cmake
CP ?= cp
ECHO ?= echo
MAKE ?= make
MKDIR ?= mkdir

EBBRTSYSROOT ?= $(abspath $(EBBRT_SYSROOT))
CMAKE_TOOLCHAIN_FILE ?= $(EBBRTSYSROOT)/usr/misc/ebbrt.cmake
BAREMETAL_PREFIX_PATH= $(EBBRTSYSROOT)/usr/

BUILD_PATH ?= $(MYDIR)
DEBUG_PATH ?= $(BUILD_PATH)/Debug
RELEASE_PATH ?= $(BUILD_PATH)/Release
BAREMETAL_DEBUG_DIR ?= $(DEBUG_PATH)
BAREMETAL_RELEASE_DIR ?= $(RELEASE_PATH)

all: Debug Release
native: native-debug native-release
Debug: native-debug 
Release: native-release 

# ENVIRONMENT VARIABLES
check-ebbrt-sysroot:
ifndef EBBRT_SYSROOT
	$(error EBBRT_SYSROOT is undefined)
endif

$(BUILD_PATH):
	$(MKDIR) $@

$(DEBUG_PATH): | $(BUILD_PATH)
	$(MKDIR) $@

$(RELEASE_PATH): | $(BUILD_PATH)
	$(MKDIR) $@

ifneq ($(DEBUG_PATH), $(BAREMETAL_DEBUG_DIR))
$(BAREMETAL_DEBUG_DIR): | $(DEBUG_PATH)
	$(MKDIR) $@
endif

ifneq ($(RELEASE_PATH), $(BAREMETAL_RELEASE_DIR))
$(BAREMETAL_RELEASE_DIR): | $(RELEASE_PATH)
	$(MKDIR) $@
endif

native-debug: | check-ebbrt-sysroot $(BAREMETAL_DEBUG_DIR)
	$(CD) $(BAREMETAL_DEBUG_DIR) && \
		EBBRT_SYSROOT=$(EBBRTSYSROOT) $(CMAKE) -DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_PREFIX_PATH=$(BAREMETAL_PREFIX_PATH) \
		-DCMAKE_TOOLCHAIN_FILE=$(CMAKE_TOOLCHAIN_FILE) $(MYDIR) && $(MAKE)

native-release: | check-ebbrt-sysroot $(BAREMETAL_RELEASE_DIR)
	$(CD) $(BAREMETAL_RELEASE_DIR) && \
		EBBRT_SYSROOT=$(EBBRTSYSROOT) $(CMAKE) -DCMAKE_BUILD_TYPE=Release  \
		-DCMAKE_PREFIX_PATH=$(BAREMETAL_PREFIX_PATH) \
		-DCMAKE_TOOLCHAIN_FILE=$(CMAKE_TOOLCHAIN_FILE) $(MYDIR) && \
		$(MAKE)


clean:
	$(MAKE) clean -C $(BAREMETAL_DEBUG_DIR) && \
	$(MAKE) clean -C $(BAREMETAL_RELEASE_DIR)

.PHONY: Debug Release all clean native native-debug native-release 
