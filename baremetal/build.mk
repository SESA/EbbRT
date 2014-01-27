common = $(baremetal)/../common/
VPATH = $(baremetal) $(common)

CXX = $(baremetal)/ext/toolchain/bin/x86_64-pc-ebbrt-g++
CC = $(baremetal)/ext/toolchain/bin/x86_64-pc-ebbrt-gcc
CAPNP = capnp

INCLUDES = -I $(baremetal)/src/include
INCLUDES += -I $(baremetal)/../common/src/include
INCLUDES += -I $(CURDIR)/include
INCLUDES += -I $(baremetal)/ext/acpica/include
INCLUDES += -I $(baremetal)/ext/boost/include
INCLUDES += -I $(baremetal)/ext/lwip/include
INCLUDES += -I $(baremetal)/ext/tbb/include
INCLUDES += -I $(baremetal)/ext/capnp/include
INCLUDES += -iquote $(baremetal)/ext/lwip/include/ipv4/

CPPFLAGS = -U ebbrt -MD -MT $@ -MP $(optflags) -Wall -Werror \
	-fno-stack-protector $(INCLUDES)
CXXFLAGS = -std=gnu++11
CFLAGS = -std=gnu99
ASFLAGS = -MD -MT $@ -MP $(optflags) -DASSEMBLY

CAPNPS := $(addprefix src/,$(notdir $(wildcard $(baremetal)/../common/src/*.capnp)))
CAPNP_OBJECTS := $(CAPNPS:.capnp=.capnp.o)
CAPNP_CXX := $(CAPNPS:.capnp=.capnp.c++)
CAPNP_H := $(CAPNPS:.capnp=.capnp.h)
CAPNP_H_MOVE := $(addprefix include/ebbrt/,$(notdir $(CAPNP_H)))
CAPNP_GENS := $(CAPNP_CXX) $(CAPNP_H) $(CAPNP_H_MOVE) $(CAPNP_OBJECTS)

CXX_SRCS := $(addprefix src/,$(notdir $(wildcard $(baremetal)/src/*.cc))) \
	$(addprefix src/,$(notdir $(wildcard $(common)/src/*.cc)))
CXX_OBJECTS := $(CXX_SRCS:.cc=.o)

ASM_SRCS := $(addprefix src/,$(notdir $(wildcard $(baremetal)/src/*.S)))
ASM_OBJECTS := $(ASM_SRCS:.S=.o)

quiet = $(if $V, $1, @echo " $2"; $1)
very-quiet = $(if $V, $1, @$1)

makedir = $(call very-quiet, mkdir -p $(dir $@))
build-cxx = $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
q-build-cxx = $(call quiet, $(build-cxx), CXX $@)
build-c = $(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
q-build-c = $(call quiet, $(build-c), CC $@)
build-s = $(CXX) $(CPPFLAGS) $(ASFLAGS) -c -o $@ $<
q-build-s = $(call quiet, $(build-s), AS $@)

objects = $(CXX_OBJECTS) $(ASM_OBJECTS)
objects += $(acpi_objects)
objects += $(tbb_objects)
objects += $(lwip_objects)
objects += $(capnp_objects)
objects += $(kj_objects)

tbb_sources := $(shell find $(baremetal)/ext/tbb -type f -name '*.cpp')
tbb_objects = $(patsubst $(baremetal)/%.cpp, %.o, $(tbb_sources))

$(tbb_objects): CPPFLAGS += -iquote $(baremetal)/ext/tbb/include

acpi_sources := $(shell find $(baremetal)/ext/acpica/components -type f -name '*.c')
acpi_objects = $(patsubst $(baremetal)/%.c, %.o, $(acpi_sources))

$(acpi_objects): CFLAGS += -fno-strict-aliasing -Wno-strict-aliasing \
	-Wno-unused-but-set-variable -DACPI_LIBRARY

lwip_sources := $(filter-out %icmp6.c %inet6.c %ip6_addr.c %ip6.c,$(shell find $(baremetal)/ext/lwip -type f -name '*.c'))
lwip_objects = $(patsubst $(baremetal)/%.c, %.o, $(lwip_sources))

$(lwip_objects): CFLAGS += -Wno-address

capnp_sources := $(shell find $(baremetal)/ext/capnp/src/capnp -type f -name '*.c++')
capnp_objects = $(patsubst $(baremetal)/%.c++, %.o, $(capnp_sources))

kj_sources := $(shell find $(baremetal)/ext/capnp/src/kj -type f -name '*.c++')
kj_objects = $(patsubst $(baremetal)/%.c++, %.o, $(kj_sources))

$(kj_objects): CXXFLAGS += -Wno-unused-variable

all: ebbrt.iso

$(CXX_OBJECTS): $(CAPNP_OBJECTS)

.PRECIOUS: $(CAPNP_GENS)

.PHONY: all clean

.SUFFIXES:

strip = strip -s $< -o $@
mkrescue = grub-mkrescue -o $@ -graft-points boot/ebbrt=$< \
	boot/grub/grub.cfg=$(baremetal)/misc/grub.cfg


ebbrt.iso: ebbrt.elf.stripped
	$(call quiet, $(mkrescue), MKRESCUE $@)

ebbrt.elf.stripped: ebbrt.elf
	$(call quiet, $(strip), STRIP $@)

# ebbrt.elf32: ebbrt.elf
# 	$(call quiet,objcopy -O elf32-i386 $< $@, OBJCOPY $@)

LDFLAGS := -Wl,-n,-z,max-page-size=0x1000 $(optflags)
ebbrt.elf: $(objects) src/ebbrt.ld
	$(call quiet, $(CXX) $(LDFLAGS) -o $@ $(objects) \
		-T $(baremetal)/src/ebbrt.ld $(runtime_objects), LD $@)

clean:
	-$(RM) $(wildcard $(CAPNP_GENS) $(objects) ebbrt.elf $(shell find -name '*.d'))

%.capnp.h %.capnp.c++: %.capnp
	$(makedir)
	$(call quiet, $(CAPNP) compile -oc++:$(CURDIR) --src-prefix=$(baremetal)/../common $<, CAPNP $<)
	$(call very-quiet, mkdir -p $(dir $(filter %$(notdir $<.h),$(CAPNP_H_MOVE))))
	$(call quiet, cp $(filter %$(notdir $<.h),$(CAPNP_H)) $(filter %$(notdir $<.h),$(CAPNP_H_MOVE)), CP $<.h)

%.capnp.o: %.capnp.c++
	$(makedir)
	$(q-build-cxx)

%.o: %.cc
	$(makedir)
	$(q-build-cxx)

%.o: %.cpp
	$(makedir)
	$(q-build-cxx)

%.o: %.c++
	$(makedir)
	$(q-build-cxx)

%.o: %.c
	$(makedir)
	$(q-build-c)

%.o: %.S
	$(makedir)
	$(q-build-s)

-include $(shell find -name '*.d')
