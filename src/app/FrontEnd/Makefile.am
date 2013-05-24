EXTRA_PROGRAMS += src/app/FrontEnd/FrontEnd

src_app_FrontEnd_FrontEnd_SOURCES = \
	src/app/FrontEnd/FrontEnd.cpp 

src_app_FrontEnd_FrontEnd_ebbs = \
	src/ebb/EbbManager/libPrimitiveEbbManager.a \
	src/ebb/MemoryAllocator/libSimpleMemoryAllocator.a

src_app_FrontEnd_FrontEnd_LDFLAGS = \
	$(app_linkflags) \
	-Wl,-whole-archive \
	$(ebbrt_libs) \
	$(src_app_FrontEnd_FrontEnd_ebbs) \
	-Wl,-no-whole-archive

src_app_FrontEnd_FrontEnd_LDADD = \
	libebbrt.a

src_app_FrontEnd_FrontEnd_DEPENDENCIES = \
	libebbrt.a \
	$(app_deps) \
	$(ebbrt_libs) \
	$(src_app_FrontEnd_FrontEnd_ebbs)
