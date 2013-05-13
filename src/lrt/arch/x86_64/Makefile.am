lrt_arch_x86_64_sources = \
	src/lrt/arch/x86_64/boot.cpp \
	src/lrt/arch/x86_64/boot.hpp \
	src/lrt/arch/x86_64/bootasm.S \
	src/lrt/arch/x86_64/default_func.S \
	src/lrt/arch/x86_64/event.cpp \
	src/lrt/arch/x86_64/event.hpp \
	src/lrt/arch/x86_64/isr.S \
	src/lrt/arch/x86_64/mem.cpp \
	src/lrt/arch/x86_64/smp.S \
	src/lrt/arch/x86_64/trans.cpp

if ARCH_X86_64
ebbrt_DEPENDENCIES += src/lrt/arch/x86_64/link.ld
endif

EXTRA_DIST += src/lrt/arch/x86_64/link.ld
