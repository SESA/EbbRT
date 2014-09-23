MYDIR := $(dir $(lastword $(MAKEFILE_LIST)))

EBBRT_TARGET := net
EBBRT_APP_OBJECTS := net.o
EBBRT_APP_VPATH := $(abspath $(MYDIR)../src)
EBBRT_CONFIG := $(abspath $(MYDIR)../src/ebbrtcfg.h)

include $(abspath ../../../../ebbrtbaremetal.mk)
