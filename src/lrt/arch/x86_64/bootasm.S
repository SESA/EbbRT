/*
 * Copyright (C) 2012 by Project SESA, Boston University
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

              .section .init.text, "ax"
.code32
.globl _start
_start:
              cli
              cmpl $0x2BADB002, %eax //Multiboot Magic
              jne _start.no_multiboot
              mov %ebx, %esi
_start.setup_stack:
              //setup stack
              lea _boot_stack, %esp
              mov 0, %ebp

_start.check_if_long_mode:
              //detect if we can cpuid
              //this is done by seeing if we can flip the ID bit in flags
              pushf //store flags register
              pop %eax //pop it into eax
              mov %eax, %ecx //store into ecx
              xor $(1 << 21), %eax //flip the ID bit
              push %eax //store onto stack
              popf //restore flags register
              pushf //store flags register
              pop %eax //pop it into eax
              push %ecx //push ecx
              popf //restores flags to original value
              xor %eax, %ecx //should be non-zero if we flipped it
              jz _start.no_cpuid

              //we can call cpuid
              //do we have extended features cpuid?
              mov $0x80000000, %eax
              cpuid
              cmp $0x80000001, %eax
              jb _start.no_ext_features

              //we have extended features
              //do we have long mode?
              mov $0x80000001, %eax
              cpuid
              test $(1 << 29), %edx
              jz _start.no_long_mode

_start.init_page_tables:
              //clear page tables first
              lea _pml4, %edi
              xor %eax, %eax
              mov $(6 * (1 << 10)), %ecx //6k times we will load 4 bytes
              rep stosl

              //point first pml4 entry to pdpt
              lea _pdpt, %eax
              add $0x3, %eax //present, read/write
              mov %eax, _pml4

              //point first 4 pdpt entries to pdir entries
              lea _pdir, %eax
              add $0x3, %eax //present.read/write
              lea _pdpt, %edi
              mov $4, %ecx //loop 4 times
_start.set_pdpt_entry:
              mov %eax, (%edi)
              add $0x1000, %eax
              add $0x8, %edi
              loop _start.set_pdpt_entry

              //identity map pdir entries
              mov $0x83, %eax //present, read/write, page size
              lea _pdir, %edi
              mov $2048, %ecx //loop 2048 times
_start.set_pdir_entry:
              mov %eax, (%edi)
              add $(1 << 21), %eax
              add $0x8, %edi
              loop _start.set_pdir_entry

_start.enable_paging:
              //point to the pml4
              lea _pml4, %eax
              mov %eax, %cr3

              //enable pae-paging
              mov %cr4, %eax
              or $(1 << 5), %eax
              mov %eax, %cr4

              //paging is setup, but not enabled
              //set long mode
              mov $0xC0000080, %ecx //read EFER MSR
              rdmsr
              or $(1 << 8), %eax //set LM-bit
              wrmsr

              //now turn on paging
              mov %cr0, %eax
              or $(1 << 31), %eax //set PG-bit
              mov %eax, %cr0

_start.jump_to_64:
              //paging is now enabled, we are executing in long-mode with
              // 32 bit compatibility mode
              //enable SSE instructions
              mov %cr0, %eax
              and $0x9FFFFFFB, %eax //clear CR0.CD, CR0.NW, CR0.EM
              or $0x22, %eax //set CR0.NE, CR0.MP
              mov %eax, %cr0
              mov %cr4, %eax
              or $0x600, %eax //set CR4.OSXMMEXCPT and CR4.OSFXSR
              mov %eax, %cr4

              //load gdt and jump
              lgdt _gdt_pointer
              ljmp $0x8, $_start.complete
.code64
_start.complete:
              mov %esi, %edi //pass multiboot struct pointer
              call _init_arch
_start.hang:
              jmp _start.hang

_start.no_multiboot:
              jmp _start.no_multiboot

_start.no_cpuid:

              jmp _start.no_cpuid

_start.no_ext_features:
              jmp _start.no_ext_features

_start.no_long_mode:
              jmp _start.no_long_mode

.section .init.data, "aw"
.globl _gdt
_gdt:
//NULL descriptor
.word 0 //limit (low)
.word 0 //base (low)
.byte 0 //base (middle)
.byte 0 //access
.byte 0 //granularity
.byte 0 //base (high)
//Code descriptor
.word 0 //limit (low)
.word 0 //base (low)
.byte 0 //base (middle)
.byte 0x98 //access, present, not system, code - execute only
.byte 0x20 //granularity, long mode
.byte 0 //base (high)
//Data descriptor
.word 0 //limit (low)
.word 0 //base (low)
.byte 0 //base (middle)
.byte 0x90 //access, present, not system
.byte 0 //granularity
.byte 0 //base (high)

.globl _gdt_pointer
_gdt_pointer:
.word . - _gdt - 1 //limit
.quad _gdt //base
.globl _gdt_pointer_end
_gdt_pointer_end:

//set boot stack, the symbol points to the beginning of the stack
.align 16
.space 0x1000
_boot_stack:

.align 4096
.globl _pml4
_pml4:
.space 0x1000

_pdpt:
.space 0x1000

_pdir:
.space 0x4000
