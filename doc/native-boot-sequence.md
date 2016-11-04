This page documents the initial boot sequence for EbbRT native runtime. 

Phase 1 of the boot sequence begins when control is received from the bootloader and continues until the event loop is initialized and the first event is exectued. Phase 2 begins with this initial event which complete the local initialization and begins the execution of the application.

# Phase 1
## Entry
System initilzation begins on core 0 once control is handed off from the bootloader. The system is entered at `start32` in [boot.S](../blob/master/src/native/Boot.S#64) and is placed in protected (32bit) mode with no paging. Register `ebx` holds a pointer to the multiboot information struct. We use a statically initialized page structure (pml4, pdpt, and pd) to identity map the first gigabyte of virtual memory. To continue, we point to this initial page structure, enable long mode, enable paging, load our static GDT (which includes a long mode code descriptor), and jump to `start64` with the long mode code descriptor. Once executing in 64 bit mode, we reset all the other segment registers, load our static boot stack, and make a call to the C function `Main` passing in a reference to the multiboot information structure.

## Main Initilization
C code execution begins at `ebbrt::Main(multiboot::Information* mbi)` in [Main.cc](../blob/master/src/native/Main.cc#L66)  

1. `console::Init`: The console is initialized which enables debugging and console output. 
2. `idt::Init`: The IDT is initialized which installs exception handlers that dump information about early exceptions.
3. `cpuid::Init`: Hardware features are detected
4. `tls::Init`: Thread local storage (i.e., support for C++ `thread_local` syntax) is initialized. 
5. `clock::Init`: The clock is initialized. This is done early to accurately record the time the system was booted.
6. `pic::Disable`: Disable legacy PIC (we used the APIC)
7. `Cpu::Create()`: The first CPU structure is initialized. This loads the `gdt` and `idt` on core 0. 

#### Memory Discovery and Page Mappings
Core 0 is still running on the initial boot stack and the first gigabyte of memory is identity mapped. We next discover free memory in the system and identity map it to virtual address space. 

8. `e820::Init`: This copies the initial memory information from the multiboot structure into our own data structure (we may unmap the multiboot structure later). The e820 map comes from the BIOS via the bootloader and describes holes in memory.
9. `early_page_allocator::Init`: This initializes a very simple page allocator which uses an intrusive red-black tree to store contiguous regions of memory ordered by address. When memory is freed back to it, it aggressively coalesces adjacent regions. Because the data structure is intrusive it requires no external memory allocation to function, the disadvantage is that all free regions of memory must be mapped.
10. `e820::ForEachUsableRegion`: This iterates over every "usable" region of memory provided in the e820 map. For each region we trim below any part that is below the end of the kernel (the kernel is loaded at 1 megabyte). We also trim above any part that is passed the initial page mapping (1 gigabyte). Then the region is added to the early page allocator. The entire region is mapped into the runtime page table (not yet enabled)
11. `vmem::EnableRuntimePageTable`: Switches from the initial 1 gigabyte mapping to the runtime mapping which identity maps all of free memory.
12. `e820::ForEachUsableRegion`: Add the rest of memory (above 1 gigabyte) to the early page allocator
13. `acpi::BootInit`: Parse the ACPI MADT and SRAT tables. The MADT lists the attached processors and interrupt routing information. We record the attached processor information for smp bringup later. The SRAT lists NUMA affinity information. We record the mapping from processor to numa node. Also, for each numa node we record the regions of memory with affinity to that node.
14. `numa::Init`: For each identified numa node, iterate through each region on that node, and notify the early page allocator of the affinity. 
15. `mem_map::Init()`: The goal here is to construct a map for each possible physical page in the system to a small descriptor which can be used to facilitate efficient memory allocation and store other information. A single level map would be huge so we use a two level hierarchical map where large "sections" need not exist if no physical page is present.

#### Ebb Initialization and Memory Allocators

16 `trans::Init()`: This function creates a per processor region of the virtual address space to be used for storing EbbRep pointers. Once this is run Ebb calls can be made (provided that we have an EbbId.)
17. `PageAllocator::Init()`: The first Ebb, a representative per numa node is constructed out of a static portion of memory. The early page allocator is instructed to release all regions of memory which are then added into the appropriate page allocator representative. This effectively retires the early page allocator. This page allocator is a buddy allocator which allows for constant time allocation and free. This is facilitates by the memory map which can be used to store information about the pages outside of the page allocator.
18. `slab::Init`: Initialize the slab allocation system. A slab allocator allocates fixed size regions of memory, using the page allocator. A slab allocator Ebb has per cpu allocators, per numa node allocators, and a root allocator. This function initializes the statically allocated slab allocators which will be used to allocate future per cpu, per numa node and root allocators of different sizes. After this function, arbitrary slab allocators can be created.
19. `GeneralPurposeAllocatorType::Init()`: Initialize the general purpose allocator which `malloc` and `free` uses. The general purpose allocator has several slab allocators of different sizes and rounds a request up to the nearest slab allocation size and invokes that slab allocator. General purpose allocation is O(1) and local in the common case.
20. `LocalIdMap::Init()`: Initialize the machine-local key-value store where keys are EbbIds. This is used to store any information necessary for resolving an Ebb miss.
21. `EbbAllocator::Init()`: Initialize the Ebb allocator which will allow for machine-local EbbIds to be allocated

### Starting the First Event
22. `VMemAllocator::Init()`: Initialize the virtual memory allocator which allows software to allocate ranges of virtual memory and install page fault handlers for them.
23. `EventManager::Init()`: Initialize the event manager which will allocate a (demand paged) stack for executing events.
24. `EventManager::SpawnLocal()`: Queue a lambda function to be invoked on the first event.
25. `EventManager::StartProcessingEvents()`: Hop onto the event stack and begin processing events.

#Phase 2
## The first event
The first even executed contains the final steps of the system initialization. 

1. `__register_frame()`: initialize C++ exceptions
2. `apic::Init()`: Allow the local APIC to be used
3. `Timer::Init()`: Initialize the interrupt driven timer
4. `smp::Init()`: Bring up the previously identified secondary processors. 

## SMP bring-up
Each secondary processor goes through a indivitial initialization process similar to that of core 0, (initialize their local apic, thread local storage, translation system, etc.) and then executes an initial event. In this event each process, including that of the main processor, enters a barrier and waits for all other processors to be initialized. This guarantees that all cores be able to process events before the main processor finished the function.

## Networking & Runtime
## Start AppMain

