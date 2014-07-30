all: $(EBBRT_TARGET).elf32

ifndef EBBRT_TARGET
  $(error You must set EBBRT_TARGET to the target name)
endif

ifndef EBBRT_APP_OBJECTS
  $(error You must set EBBRT_APP_OBJECTS to the set of object files to be linked in)
endif

ifndef EBBRT_CONFIG
  $(error You must set EBBRT_CONFIG to the location of the config header for the app)
endif

ifeq ($(wildcard $(EBBRT_CONFIG)),)
  $(error Cannot find config file $(EBBRT_CONFIG))
endif

EBBRT_OPTFLAGS ?= -O2

EBBRT_PATH := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))..)

EBBRT_BAREMETAL_PATH := $(EBBRT_PATH)/baremetal/
EBBRT_COMMON_PATH := $(EBBRT_PATH)/common/
VPATH := $(EBBRT_BAREMETAL_PATH) $(EBBRT_BAREMETAL_PATH)/src $(EBBRT_COMMON_PATH) $(EBBRT_COMMON_PATH)/src $(EBBRT_APP_VPATH)

EBBRT_CXX := $(EBBRT_BAREMETAL_PATH)/ext/toolchain/bin/x86_64-pc-ebbrt-g++
EBBRT_CC := $(EBBRT_BAREMETAL_PATH)/ext/toolchain/bin/x86_64-pc-ebbrt-gcc
EBBRT_CAPNP := capnp

EBBRT_INCLUDES := \
	-I $(EBBRT_BAREMETAL_PATH)/src/include \
	-I $(EBBRT_BAREMETAL_PATH)/../common/src/include \
	-I $(EBBRT_BAREMETAL_PATH)/ext/acpica/include \
	-I $(EBBRT_BAREMETAL_PATH)/ext/boost/include \
	-I $(EBBRT_BAREMETAL_PATH)/ext/lwip/include \
	-I $(EBBRT_BAREMETAL_PATH)/ext/tbb/include \
	-I $(EBBRT_BAREMETAL_PATH)/ext/capnp/include \
	-I $(EBBRT_BAREMETAL_PATH)/ext/fdt/include \
	-iquote $(EBBRT_BAREMETAL_PATH)/ext/lwip/include/ipv4/ \
	-iquote $(CURDIR) \
	-include $(EBBRT_CONFIG) \
	$(EBBRT_APP_INCLUDES)

EBBRT_CPPFLAGS = -U ebbrt -MD -MT $@ -MP $(EBBRT_OPTFLAGS) -Wall -Werror \
	-fno-stack-protector $(EBBRT_INCLUDES)
EBBRT_CXXFLAGS = -std=gnu++11 -ftemplate-backtrace-limit=0
EBBRT_CFLAGS = -std=gnu99
EBBRT_ASFLAGS = -MD -MT $@ -MP $(EBBRT_OPTFLAGS) -DASSEMBLY

EBBRT_CAPNPS := $(notdir $(wildcard $(EBBRT_COMMON_PATH)/src/*.capnp))
EBBRT_CAPNP_OBJECTS := $(EBBRT_CAPNPS:.capnp=.capnp.o)
EBBRT_CAPNP_CXX := $(EBBRT_CAPNPS:.capnp=.capnp.c++)
EBBRT_CAPNP_H := $(EBBRT_CAPNPS:.capnp=.capnp.h)
EBBRT_CAPNP_GENS := $(EBBRT_CAPNP_CXX) $(EBBRT_CAPNP_H) $(EBBRT_CAPNP_OBJECTS)

EBBRT_CXX_SRCS := \
	$(notdir $(wildcard $(EBBRT_BAREMETAL_PATH)/src/*.cc)) \
	$(notdir $(wildcard $(EBBRT_COMMON_PATH)/src/*.cc))
EBBRT_CXX_OBJECTS := $(EBBRT_CXX_SRCS:.cc=.o)

EBBRT_ASM_SRCS := $(notdir $(wildcard $(EBBRT_BAREMETAL_PATH)/src/*.S))
EBBRT_ASM_OBJECTS := $(EBBRT_ASM_SRCS:.S=.o)

ebbrt_quiet = $(if $V, $1, @echo " $2"; $1)
ebbrt_very-quiet = $(if $V, $1, @$1)

ebbrt_makedir = $(call ebbrt_very-quiet, mkdir -p $(dir $@))
ebbrt_build-cxx = $(EBBRT_CXX) $(EBBRT_CPPFLAGS) $(EBBRT_CXXFLAGS) -c -o $@ $<
ebbrt_q-build-cxx = $(call ebbrt_quiet, $(ebbrt_build-cxx), CXX $@)
ebbrt_build-c = $(EBBRT_CC) $(EBBRT_CPPFLAGS) $(EBBRT_CFLAGS) -c -o $@ $<
ebbrt_q-build-c = $(call ebbrt_quiet, $(ebbrt_build-c), CC $@)
ebbrt_build-s = $(EBBRT_CXX) $(EBBRT_CPPFLAGS) $(EBBRT_ASFLAGS) -c -o $@ $<
ebbrt_q-build-s = $(call ebbrt_quiet, $(ebbrt_build-s), AS $@)

tbb_sources := $(shell find $(EBBRT_BAREMETAL_PATH)/ext/tbb -type f -name '*.cpp')
tbb_objects := $(patsubst $(EBBRT_BAREMETAL_PATH)/%.cpp, %.o, $(tbb_sources))

$(tbb_objects): EBBRT_CPPFLAGS += -iquote $(EBBRT_BAREMETAL_PATH)/ext/tbb/include

acpi_sources := $(shell find $(EBBRT_BAREMETAL_PATH)/ext/acpica/components \
	-type f -name '*.c')
acpi_objects := $(patsubst $(EBBRT_BAREMETAL_PATH)/%.c, %.o, $(acpi_sources))

$(acpi_objects): EBBRT_CFLAGS += -fno-strict-aliasing -Wno-strict-aliasing \
	-Wno-unused-but-set-variable -DACPI_LIBRARY

lwip_sources := $(filter-out %icmp6.c %inet6.c %ip6_addr.c %ip6.c,$(shell find \
	$(EBBRT_BAREMETAL_PATH)/ext/lwip -type f -name '*.c'))
lwip_objects := $(patsubst $(EBBRT_BAREMETAL_PATH)/%.c, %.o, $(lwip_sources))

$(lwip_objects): EBBRT_CFLAGS += -Wno-address -Wno-unused-but-set-variable -Wno-array-bounds -Wno-unused-variable

capnp_sources := $(shell find $(EBBRT_BAREMETAL_PATH)/ext/capnp/src/capnp -type f -name '*.c++')
capnp_objects := $(patsubst $(EBBRT_BAREMETAL_PATH)/%.c++, %.o, $(capnp_sources))

$(capnp_objects): EBBRT_CPPFLAGS := $(filter-out -flto,$(EBBRT_CPPFLAGS))

kj_sources := $(shell find $(EBBRT_BAREMETAL_PATH)/ext/capnp/src/kj -type f -name '*.c++')
kj_objects := $(patsubst $(EBBRT_BAREMETAL_PATH)/%.c++, %.o, $(kj_sources))

$(kj_objects): EBBRT_CXXFLAGS += -Wno-unused-variable
$(kj_objects): EBBRT_CPPFLAGS := $(filter-out -flto,$(EBBRT_CPPFLAGS))

fdt_sources := $(shell find $(EBBRT_BAREMETAL_PATH)/ext/fdt -type f -name '*.c')
fdt_objects := $(patsubst $(EBBRT_BAREMETAL_PATH)/%.c, %.o, $(fdt_sources))

EBBRT_OBJECTS := \
	$(EBBRT_CXX_OBJECTS) \
	$(EBBRT_ASM_OBJECTS) \
	$(acpi_objects) \
	$(tbb_objects) \
	$(capnp_objects) \
	$(kj_objects) \
	$(fdt_objects)

$(EBBRT_CXX_OBJECTS): $(EBBRT_CAPNP_OBJECTS)

EBBRT_APP_CAPNP_OBJECTS := $(EBBRT_APP_CAPNPS:.capnp=.capnp.o)
EBBRT_APP_CAPNP_CXX := $(EBBRT_APP_CAPNPS:.capnp=.capnp.c++)
EBBRT_APP_CAPNP_H := $(EBBRT_APP_CAPNPS:.capnp=.capnp.h)
EBBRT_APP_CAPNP_GENS := \
	$(EBBRT_APP_CAPNP_CXX) $(EBBRT_APP_CAPNP_H) \
	$(EBBRT_APP_CAPNP_OBJECTS)

$(EBBRT_APP_OBJECTS): $(EBBRT_APP_CAPNP_OBJECTS)

.PRECIOUS: $(EBBRT_CAPNP_GENS) $(EBBRT_APP_CAPNP_GENS) $(EBBRT_TARGET).elf %.o

.PHONY: all clean

.SUFFIXES:

ebbrt_strip = strip -s $< -o $@
ebbrt_mkrescue = grub-mkrescue -o $@ -graft-points boot/ebbrt=$< \
	boot/grub/grub.cfg=$(EBBRT_BAREMETAL_PATH)/misc/grub.cfg


%.iso: %.elf.stripped
	$(call ebbrt_quiet, $(ebbrt_mkrescue), MKRESCUE $@)

%.elf.stripped: %.elf
	$(call ebbrt_quiet, $(ebbrt_strip), STRIP $@)

%.elf32: %.elf.stripped
	$(call ebbrt_quiet,objcopy -O elf32-i386 $< $@, OBJCOPY $@)

EBBRT_LDFLAGS := -Wl,-n,-z,max-page-size=0x1000 $(EBBRT_OPTFLAGS)

comma := ,

%.elf: $(EBBRT_APP_OBJECTS) $(EBBRT_OBJECTS) src/ebbrt.ld
	$(call ebbrt_quiet, $(EBBRT_CXX) $(EBBRT_LDFLAGS) \
	-o $@ -Wl$(comma)--whole-archive $(EBBRT_APP_OBJECTS) \
	-Wl$(comma)--no-whole-archive $(EBBRT_OBJECTS) $(EBBRT_APP_LINK) \
	-T $(EBBRT_BAREMETAL_PATH)/src/ebbrt.ld, LD $@)

clean:
	-$(RM) $(wildcard $(EBBRT_CAPNP_GENS) $(EBBRT_OBJECTS) $(EBBRT_APP_CAPNP_GENS) \
	$(EBBRT_APP_OBJECTS) \
	$(EBBRT_TARGET).iso $(EBBRT_TARGET).elf $(EBBRT_TARGET).elf.stripped \
	$(EBBRT_TARGET).elf32 $(shell find -name '*.d'))

%.capnp.h %.capnp.c++: %.capnp
	$(ebbrt_makedir)
	$(call ebbrt_quiet, $(EBBRT_CAPNP) compile -oc++:$(CURDIR) \
		--src-prefix=$(dir $<) $<, CAPNP $<)

%.capnp.o: %.capnp.c++
	$(ebbrt_makedir)
	$(ebbrt_q-build-cxx)

%.o: %.cc
	$(ebbrt_makedir)
	$(ebbrt_q-build-cxx)

%.o: %.cpp
	$(ebbrt_makedir)
	$(ebbrt_q-build-cxx)

%.o: %.c++
	$(ebbrt_makedir)
	$(ebbrt_q-build-cxx)

%.o: %.c
	$(ebbrt_makedir)
	$(ebbrt_q-build-c)

%.o: %.S
	$(ebbrt_makedir)
	$(ebbrt_q-build-s)

-include $(shell find -name '*.d')
