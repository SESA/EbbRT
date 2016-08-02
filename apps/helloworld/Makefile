MYDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

CD ?= cd
CMAKE ?= cmake
CP ?= cp
ECHO ?= echo
MAKE ?= make
MKDIR ?= mkdir

EBBRTSYSROOT ?= $(abspath $(EBBRT_SYSROOT))
CMAKE_TOOLCHAIN_FILE ?= $(EBBRTSYSROOT)/usr/misc/ebbrt.cmake

BUILD_PATH ?= $(MYDIR)
DEBUG_PATH ?= $(BUILD_PATH)/Debug
RELEASE_PATH ?= $(BUILD_PATH)/Release
BAREMETAL_DEBUG_DIR ?= $(DEBUG_PATH)/bm
BAREMETAL_RELEASE_DIR ?= $(RELEASE_PATH)/bm
HOSTED_DEBUG_DIR ?= $(DEBUG_PATH)
HOSTED_RELEASE_DIR ?= $(RELEASE_PATH)

all: Debug Release
hosted: hosted-debug hosted-release
baremetal: baremetal-debug baremetal-release
Debug: baremetal-debug hosted-debug
Release: baremetal-release hosted-release

# ENVIRONMENT VARIABLES
check-ebbrt-sysroot:
ifndef EBBRT_SYSROOT
	$(error EBBRT_SYSROOT is undefined)
endif

check-ebbrt-src:
ifndef EBBRT_SRCDIR
	$(warning EBBRT_SRCDIR is undefined)
endif

$(BUILD_PATH):
	$(MKDIR) $@

$(DEBUG_PATH): | $(BUILD_PATH)
	$(MKDIR) $@

$(RELEASE_PATH): | $(BUILD_PATH)
	$(MKDIR) $@

$(BAREMETAL_DEBUG_DIR): | $(DEBUG_PATH)
	$(MKDIR) $@

$(BAREMETAL_RELEASE_DIR): | $(RELEASE_PATH)
	$(MKDIR) $@

$(HOSTED_DEBUG_DIR): | $(DEBUG_PATH)
	$(MKDIR) $@

$(HOSTED_RELEASE_DIR): | $(RELEASE_PATH)
	$(MKDIR) $@

baremetal-debug: | check-ebbrt-src check-ebbrt-sysroot $(BAREMETAL_DEBUG_DIR)
	$(CD) $(BAREMETAL_DEBUG_DIR) && \
		EBBRT_SYSROOT=$(EBBRTSYSROOT) $(CMAKE) -DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_TOOLCHAIN_FILE=$(CMAKE_TOOLCHAIN_FILE) $(MYDIR) && $(MAKE)

baremetal-release: | check-ebbrt-src check-ebbrt-sysroot $(BAREMETAL_RELEASE_DIR)
	$(CD) $(BAREMETAL_RELEASE_DIR) && \
		EBBRT_SYSROOT=$(EBBRTSYSROOT) $(CMAKE) -DCMAKE_BUILD_TYPE=Release  \
		-DCMAKE_TOOLCHAIN_FILE=$(CMAKE_TOOLCHAIN_FILE) $(MYDIR) && \
		$(MAKE)

hosted-debug: | check-ebbrt-src $(HOSTED_DEBUG_DIR)
	$(CD) $(HOSTED_DEBUG_DIR) && $(CMAKE) -DCMAKE_BUILD_TYPE=Debug \
		$(MYDIR) && $(MAKE)

hosted-release: | check-ebbrt-src $(HOSTED_RELEASE_DIR)
	$(CD) $(HOSTED_RELEASE_DIR) && $(CMAKE) -DCMAKE_BUILD_TYPE=Release  \
		$(MYDIR) && $(MAKE)

clean:
	$(MAKE) clean -C $(HOSTED_DEBUG_DIR) && \
	$(MAKE) clean -C $(HOSTED_RELEASE_DIR) && \
	$(MAKE) clean -C $(BAREMETAL_DEBUG_DIR) && \
	$(MAKE) clean -C $(BAREMETAL_RELEASE_DIR)

.PHONY: Debug Release all clean baremetal baremetal-debug baremetal-release hosted hosted-debug hosted-release
