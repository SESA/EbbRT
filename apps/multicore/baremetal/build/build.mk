MYDIR := $(dir $(lastword $(MAKEFILE_LIST)))

EBBRT_TARGET := multicore
EBBRT_APP_OBJECTS := multicore.o
EBBRT_APP_VPATH := $(abspath $(MYDIR)../src)
EBBRT_CONFIG := $(abspath $(MYDIR)../src/ebbrtcfg.h)

include $(abspath ../../../../ebbrtbaremetal.mk)
