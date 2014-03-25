ifeq ($(strip ${EBBRT_SRCDIR}),)
 $(error EBBRT_SRCDIR not set)
else
 $(info EBBRT_SRCDIR set to $(EBBRT_SRCDIR))
endif

baremetal = ${EBBRT_SRCDIR}/baremetal

ifeq ($(strip ${EBBRT_BUILDTYPE}),Debug)
  EBBRT_OPTFLAGS ?=-g3 -O0
else ifeq ($(strip ${EBBRT_BUILDTYPE}),Release)
  EBBRT_OPTFLAGS ?= -O4 -DNDEBUG -flto
else 
  $(error EBBRT_BUILDTYPE must be set to either Debug or Release)
endif

ifeq ($(strip ${EBBRT_OPTFLAGS}),)
 $(error You must set EBBRT_OPTFLAGS as you need. eg. -O4 -flto -DNDEBUG \
	for an optimized build or -g3 -O0 for a non-optimized debug build)
endif	

EBBRT_OPTFLAGS += -D__EBBRT_BM__ -I$(abspath ../../src) -I$(abspath ../../../../src/baremetal)

# Places to find source files assume that make is excuting in one of the
# build directories
# ../../src : barmetal source for this application
# ../../../src: Common source for this application both baremetal and hosted
# ../../../../src: Common hosted and baremetal source files that are provide for all apps
# ../../../../src/bm: Common baremetal source files that are provided for all applications to use
EBBRT_APP_VPATH := $(abspath ../../src):$(abspath ../../../src):$(abspath ../../../../src):$(abspath ../../../../src/baremetal)

EBBRT_CONFIG = $(abspath ../../src/ebbrtcfg.h)

include $(baremetal)/build.mk



.PHONY: distclean

distclean: clean
	-$(RM) -rf $(wildcard include ext sys src $(EBBRT_APP_OBJECTS) $(EBBRT_TARGET).*)
