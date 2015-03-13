MYDIR := $(dir $(lastword $(MAKEFILE_LIST)))

EBBRT_TARGET := netpipe
EBBRT_APP_OBJECTS := netpipe.o
EBBRT_APP_VPATH := $(abspath $(MYDIR)../src)
EBBRT_CONFIG := $(abspath $(MYDIR)../src/ebbrtcfg.h)

include $(abspath ../../../../ebbrtbaremetal.mk)
