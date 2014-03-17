MYDIR := $(dir $(lastword $(MAKEFILE_LIST)))

app_sources := \
	helloworld.cc \
	Printer.cc 

target := helloworld

include $(abspath $(MYDIR)../../../ebbrthosted.mk)
