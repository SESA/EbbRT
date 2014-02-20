ebbrt_dir := ../../../../../
ebbrt_lib := $(ebbrt_dir)/hosted/build/libEbbRT.a

helloworld_src_dir := ../../

VPATH := $(helloworld_src_dir)

helloworld_sources := \
	helloworld.cc

helloworld_objects := $(helloworld_sources:.cc=.o)

INCLUDES := \
	-I $(ebbrt_dir)/hosted/src/include \
	-I $(ebbrt_dir)/common/src/include

CXXFLAGS := -std=c++11 $(INCLUDES)

.PHONY: all clean

all: helloworld

clean:
	-$(RM) $(wildcard $(helloworld_objects) helloworld)

helloworld: $(helloworld_objects) $(ebbrt_lib)
	$(CXX) $< $(ebbrt_lib) -lboost_filesystem -lboost_system \
		-lcapnp -lkj -lfdt -pthread -o $@
