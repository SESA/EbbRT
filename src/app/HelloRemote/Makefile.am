EXTRA_PROGRAMS += src/app/HelloRemote/HelloRemote

src_app_HelloRemote_HelloRemote_SOURCES = \
	src/app/HelloRemote/HelloRemote.cpp

src_app_HelloRemote_HelloRemote_ebbs = \
	src/ebb/EbbManager/libPrimitiveEbbManager.a \
	src/ebb/MemoryAllocator/libSimpleMemoryAllocator.a \
	src/ebb/EventManager/libSimpleEventManager.a \
	src/ebb/Console/libConsole.a \
	src/ebb/MessageManager/libMessageManager.a \
	src/ebb/Ethernet/libVirtioNet.a \
	src/ebb/PCI/libPCI.a

src_app_HelloRemote_HelloRemote_LDFLAGS = \
	$(app_linkflags) \
	-Wl,-whole-archive \
	$(ebbrt_libs) \
	$(src_app_HelloRemote_HelloRemote_ebbs) \
	-Wl,-no-whole-archive

src_app_HelloRemote_HelloRemote_LDADD = \
	libebbrt.a

src_app_HelloRemote_HelloRemote_DEPENDENCIES = \
	libebbrt.a \
	$(app_deps) \
	$(ebbrt_libs) \
	$(src_app_HelloRemote_HelloRemote_ebbs)
