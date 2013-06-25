/*
  EbbRT: Distributed, Elastic, Runtime
  Copyright (C) 2013 SESA Group, Boston University

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "lrt/bare/arch/x86_64/smp.hpp"

.section .init.smp, "ax"
.code16

.globl _smp_start
_smp_start:
        cli

        //set our data segment to the same as code segment
        mov %cs, %ax
        mov %ax, %ds
        //We can't use the original gdt pointer because it is too far away
        //remember this is 16 bit code so we can only access memory inside the
        // first 2^16 bytes which is beyond our original gdt pointer
        //We copy the gdt pointer in at the end of this piece of code
        //note that we calculate its offset from _smp_start because
        //this will be treated as an absolute address in the data segment
        //which starts at _smp_start after we copy this code
        lgdt _smp_end - _smp_start

        //set PAE and PGE
        mov $0xa0, %eax
        mov %eax, %cr4

        //point cr3 to the pml4
        mov $_pml4, %eax
        mov %eax, %cr3

        //set LME bit in EFER
        mov $0xc0000080, %ecx
        rdmsr
        or $0x00000100, %eax
        wrmsr

        //Activate longmode, enable paging and protection
        mov %cr0, %eax
        or $0x80000001, %eax
        mov %eax, %cr0

        //enable SSE instructions
        mov %cr0, %eax
        and $0x9FFFFFFB, %eax //clear CR0.CD, CR0.NW, CR0.EM
        or $0x22, %eax //set CR0.NE, CR0.MP
        mov %eax, %cr0
        mov %cr4, %eax
        or $0x600, %eax //set CR4.OSXMMEXCPT and CR4.OSFXSR
        mov %eax, %cr4

        //Long jump using the long mode code segment
        //This code gets moved so we need to calculate the
        // address with segment base zero
        ljmp $0x8, $_smp_64 - _smp_start + SMP_START_ADDRESS
.code64
_smp_64:
        mov $0x0, %ax
        mov %ax, %es
        mov %ax, %ss
        mov %ax, %ds
        mov %ax, %fs
        mov %ax, %gs
        fninit //Initialize FPU
        fldcw _fpu_control_word - _smp_start + SMP_START_ADDRESS
        mov $_smp_stack + SMP_STACK_SIZE, %rsp
        mov $0x0, %rdi
        mov $_init_cpu_arch, %rax
        call *%rax
1:      jmp 1b

.globl _smp_end
_smp_end:
