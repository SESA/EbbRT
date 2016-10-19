This page documents the boot sequence for EbbRT baremetal on x86_64. If you notice any discrepancies then please edit the page. The primary objective of startup is to quickly begin execution within the context of an event. Though this initial event we complete the local initialization, connect the node to the distributed runtime, and begin execution of application code.

## Entry
The system is first entered at `start32` in [boot.S](../blob/master/baremetal/src/Boot.S). Presuming we have been booted by a multiboot compatible bootloader (e.g, grub), we are put into protected mode with no paging. Register `ebx` holds a pointer to the multiboot information struct. We have a statically initialized page structure (pml4, pdpt, and pd) these three identity map the first gigabyte of virtual memory. We point to this page structure, enable long mode, enable paging, load our static GDT (which includes a long mode code descriptor), then jump to `start64` with the long mode code descriptor. We are now executing 64 bit code. We reset all the other segment registers, load our static boot stack, and call `Main`, the first C code to execute, to which we pass along a pointer to the multiboot information structure.

## Main
`Main` in [main.cc](../blob/master/baremetal/src/Main.cc) continues initialization. 

1. `console::Init`: The console is initialized which enables debugging and information output. 
2. `idt::Init`: The IDT is initialized which installs exception handlers which will just dump information about exceptions that occur early on.
3. `cpuid::Init`: Features are discovered using `cpuid`
4. `tls::Init`: Thread local storage (support for `thread_local` variables) is initialized and can be used on the boot processor
5. `clock::Init`: The clock is initialized. This allows for reading of wall clock and should be done early to record the time the system was booted at accurately.
6. `pic::Disable`: The legacy PIC is disabled (we used the APIC)
7. `Cpu::Create()`: The first cpu structure is initialized. This loads the gdt and idt on the boot processor

### Memory Information Discovery and Page Mappings
We are still running on the initial boot stack and the first gigabyte of memory is identity mapped. Our goal is to discover what free RAM exists in the system and identity map all of it. 

1. `e820::Init`: This copies the initial memory information from the multiboot structure into our own data structure (we may unmap the multiboot structure later). The e820 map comes from the BIOS via the bootloader and describes holes in memory.
2. `early_page_allocator::Init`: This initializes a very simple page allocator which uses an intrusive red-black tree to store contiguous regions of memory ordered by address. When memory is freed back to it, it aggressively coalesces adjacent regions. Because the data structure is intrusive it requires no external memory allocation to function, the disadvantage is that all free regions of memory must be mapped.
3. `e820::ForEachUsableRegion`: This iterates over every "usable" region of memory provided in the e820 map. For each region we trim below any part that is below the end of the kernel (the kernel is loaded at 1 megabyte). We also trim above any part that is passed the initial page mapping (1 gigabyte). Then the region is added to the early page allocator. The entire region is mapped into the runtime page table (not yet enabled)
4. `vmem::EnableRuntimePageTable`: Switches from the initial 1 gigabyte mapping to the runtime mapping which identity maps all of free memory.
5. `e820::ForEachUsableRegion`: Add the rest of memory (above 1 gigabyte) to the early page allocator
6. `acpi::BootInit`: Parse the ACPI MADT and SRAT tables. The MADT lists the attached processors and interrupt routing information. We record the attached processor information for smp bringup later. The SRAT lists NUMA affinity information. We record the mapping from processor to numa node. Also, for each numa node we record the regions of memory with affinity to that node.
7. `numa::Init`: For each identified numa node, iterate through each region on that node, and notify the early page allocator of the affinity. 
8. `mem_map::Init()`: The goal here is to construct a map for each possible physical page in the system to a small descriptor which can be used to facilitate efficient memory allocation and store other information. A single level map would be huge so we use a two level hierarchical map where large "sections" need not exist if no physical page is present.

### Ebb Initialization and Memory Allocators

1. `trans::Init()`: This function creates a per processor region of the virtual address space to be used for storing EbbRep pointers. Once this is run Ebb calls can be made (provided that we have an EbbId.)
2. `PageAllocator::Init()`: The first Ebb, a representative per numa node is constructed out of a static portion of memory. The early page allocator is instructed to release all regions of memory which are then added into the appropriate page allocator representative. This effectively retires the early page allocator. This page allocator is a buddy allocator which allows for constant time allocation and free. This is facilitates by the memory map which can be used to store information about the pages outside of the page allocator.
3. `slab::Init`: Initialize the slab allocation system. A slab allocator allocates fixed size regions of memory, using the page allocator. A slab allocator Ebb has per cpu allocators, per numa node allocators, and a root allocator. This function initializes the statically allocated slab allocators which will be used to allocate future per cpu, per numa node and root allocators of different sizes. After this function, arbitrary slab allocators can be created.
4. `GeneralPurposeAllocatorType::Init()`: Initialize the general purpose allocator which `malloc` and `free` uses. The general purpose allocator has several slab allocators of different sizes and rounds a request up to the nearest slab allocation size and invokes that slab allocator. General purpose allocation is O(1) and local in the common case.
5. `LocalIdMap::Init()`: Initialize the machine-local key-value store where keys are EbbIds. This is used to store any information necessary for resolving an Ebb miss.
6. `EbbAllocator::Init()`: Initialize the Ebb allocator which will allow for machine-local EbbIds to be allocated

### Starting the First Event
1. `VMemAllocator::Init()`: Initialize the virtual memory allocator which allows software to allocate ranges of virtual memory and install page fault handlers for them.
2. `EventManager::Init()`: Initialize the event manager which will allocate a (demand paged) stack for executing events.
3. `EventManager::SpawnLocal()`: Queue a function to be invoked the next time there are no interrupts to process in the event loop. The enclosed lambda will be executed on the first event.
4. `EventManager::StartProcessingEvents()`: Hop onto the event stack and begin processing events

## The first event
The first event is a continuation of the initialization

1. `__register_frame()`: initialize C++ exceptions
2. `apic::Init()`: Allow the local APIC to be used
3. `Timer::Init()`: Initialize the interrupt driven timer
4. `smp::Init()`: Bring up the previously identified secondary processors. Each secondary processor goes through a minimal initialization (initialize their local apic, thread local storage, translation system, etc.) and then executes an initial event. In this event each process, including that of the main processor, enters a barrier and waits for all other processors to be initialized. This guarantees that all cores be able to process events before the main processor finished the function.
