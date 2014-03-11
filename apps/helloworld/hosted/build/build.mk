OPTFLAGS ?= -O2

FILE_PATH := $(dir $(CURDIR)/$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))

ebbrt_dir := $(FILE_PATH)/../../../../
ebbrt_lib := $(ebbrt_dir)/hosted/build/libEbbRT.a

helloworld_src_dir := $(FILE_PATH)/../

VPATH := $(helloworld_src_dir)

helloworld_sources := \
	helloworld.cc \
	Printer.cc

helloworld_objects := $(helloworld_sources:.cc=.o)

INCLUDES := \
	-I $(ebbrt_dir)/hosted/src/include \
	-I $(ebbrt_dir)/common/src/include

CXXFLAGS := -std=c++11 $(INCLUDES) $(OPTFLAGS)

.PHONY: all clean

all: helloworld

clean:
	-$(RM) $(wildcard $(helloworld_objects) helloworld)

helloworld: $(helloworld_objects) $(ebbrt_lib)
	$(CXX) $(OPTFLAGS) -Wl,--whole-archive $(helloworld_objects) $(ebbrt_lib) -Wl,--no-whole-archive \
	-lboost_filesystem -lboost_system -lcapnp -lkj -lfdt -pthread -o $@
