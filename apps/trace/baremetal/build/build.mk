FILE_PATH := $(dir $(CURDIR)/$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))

EBBRT_APP_VPATH := $(FILE_PATH)/../

EBBRT_CONFIG := $(FILE_PATH)/../config.h

EBBRT_TARGET := trace

EBBRT_APP_OBJECTS := \
	trace.o

include ../../../../../baremetal/build.mk
