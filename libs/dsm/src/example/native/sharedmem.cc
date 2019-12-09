//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "Printer.h"
#include "../sharedmem.h"

// a function to be called to invoke events in other cores
void test(uintptr_t addr);
// this function use a for loop to fill the physical page with desireed value
void fillInValue(int value, int iteration, volatile uint32_t * ptr);
//this function use a for loop to lazly invoked the page fault handler
void lazyMap(int value, int iteration, int multiplier, volatile uint32_t * ptr);

void AppMain() { 
  int len = 0; // number of pages need to be allocated physical allocator
  int mul = 1; // multipler for how many times more virtual pages to be allocated
  auto  ps = ebbrt::pmem::kPageSize; // page size by default
  int iteration = ((int)ps*(1<<len))/4; // number of iterations for filling the page
  int v = 0xDEADBEEF; // value that can be filled into the page
  int unmap = 0;//a flag for unmaping

  //create the instances of the page fault handler
  //and allocate the virtual memory pages which will hit page fault when dereferrenced
  printer->Print("BEGIN TO ALLOCATE THE VIRTUAL PAGE.\n");
  auto pf = std::make_unique<NewPageFaultHandler>();
  pf->setPage(len, v);
  auto vfn = ebbrt::vmem_allocator->Alloc(mul*(1<<len), std::move(pf));
  auto addr = vfn.ToAddr();
  auto ptr = (volatile uint32_t *)addr;
  lazyMap(v, iteration, mul, ptr);
  ebbrt::kprintf("GOT A PAGE VALUE IS: 0x%x\n",*ptr);


  if (unmap == 1){
  printer->Print("BEGIN TO UNMAP THE VIRTUAL PAGE.\n");
  //start to unmap the virtual region by traversing the page talbes and delete the entries
  TraversePageTable(
       ebbrt::vmem::GetPageTableRoot(), addr, addr + mul*(1 << len)*ps, 0, 4,
       [=](ebbrt::vmem::Pte& entry, uint64_t base_virt, size_t level) {
        kassert(entry.Present());
        entry.Clear();
        std::atomic_thread_fence(std::memory_order_release);
        asm volatile("invlpg (%[addr])" : : [addr] "r"(base_virt) : "memory");
      },
	[=](ebbrt::vmem::Pte& entry) {
        ebbrt::kprintf("Asked to unmap memory that wasn't mapped!\n");
        ebbrt::kabort();
        return false;
      });
  };
  
  ptr = (volatile uint32_t *)addr;
  ebbrt::kbugon( (int)*ptr != v, "wrong value for remap the virtual region!!\n");
  // the following is for spawning different events on other cores.
  //  auto f = [addr]() {
  //  ebbrt::kprintf("Hello from %u\n", (size_t)ebbrt::Cpu::GetMine());
  //  test(addr);
  //  test(addr);
  // };
  //ebbrt::event_manager->SpawnLocal(f);
  //  ebbrt::event_manager->SpawnRemote(f, 1);
}

void test(uintptr_t addr) {
  auto ptr = (volatile uint32_t *)addr;
  (void)*ptr;  
}

void fillInValue(int value, int iteration, volatile uint32_t * ptr){
  for(int i = 0; i < iteration; i++){
    *ptr = value;
    ptr++;
  }

}

void lazyMap(int value, int iteration,int multiplier,  volatile uint32_t *ptr){
  for(int i = 0; i < multiplier * iteration; i++){
    volatile auto val = *ptr;
    ebbrt::kbugon( (int)val != value, "wrong value in the virtual memory!!\n");
    ptr++;
  }

}

