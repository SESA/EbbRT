EXTRA_PROGRAMS += src/app/HelloWorld/HelloWorld

src_app_HelloWorld_HelloWorld_SOURCES = \
	src/app/HelloWorld/HelloWorld.cpp

src_app_HelloWorld_HelloWorld_ebbs = \
	src/ebb/EbbManager/libPrimitiveEbbManager.a \
	src/ebb/MemoryAllocator/libSimpleMemoryAllocator.a

src_app_HelloWorld_HelloWorld_LDFLAGS = \
	$(app_linkflags) \
	-Wl,-whole-archive \
	$(ebbrt_libs) \
	$(src_app_HelloWorld_HelloWorld_ebbs) \
	-Wl,-no-whole-archive

src_app_HelloWorld_HelloWorld_LDADD = \
	libebbrt.a

src_app_HelloWorld_HelloWorld_DEPENDENCIES = \
	libebbrt.a \
	$(app_deps) \
	$(ebbrt_libs) \
	$(src_app_HelloWorld_HelloWorld_ebbs)


