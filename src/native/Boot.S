.text
.code32

.data
.align 4096
pml4:
	.quad pdpt + 0x3
	.rept 511
	.quad 0
	.endr
pdpt:
	.quad pd + 0x3
	.rept 511
	.quad 0
	.endr
pd:
	index = 0
	.rept 512
	.quad (index << 21) | 0x83
	index = index + 1
	.endr

gdt_desc:
	.short gdt_end - gdt - 1
	.long gdt

.align 8
gdt:
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
gdt_end = .

.globl _fpu_control_word
_fpu_control_word:
.word 0x37f //enable both division by zero and invalid operand exceptions

.bss
.align 16
. = . + 16 * 1024
boot_stack_top = .

.text
.align 4        
.globl start32
start32:
        cli
        cmpl $0x2BADB002, %eax //Multiboot Magic
        jne start32.no_multiboot
	mov %ebx, %edi

        //setup stack
        lea boot_stack_top, %esp
        mov $0, %ebp

        //detect if we can cpuid
        //see if we can flip the id bit in flags
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
	jz start32.no_cpuid

	//we can call cpuid
	//do we have extended features cpuid?
	mov $0x80000000, %eax
	cpuid
	cmp $0x80000001, %eax
	jb start32.no_ext_features

	//we have extended features
	//do we have long mode?
	mov $0x80000001, %eax
	cpuid
	test $(1 << 29), %edx
	jz start32.no_long_mode        
        
	//enable PAE and SSE
	mov $0x628, %eax
	mov %eax, %cr4
	//point to the pml4
        lea pml4, %eax
        mov %eax, %cr3
        //paging is setup, but not enabled
        //set long mode
        mov $0xC0000080, %ecx //EFER MSR
	mov $0x900, %eax //NXE + LME
        mov $0, %edx
        wrmsr
        
	mov $0x80010023, %eax //set page bit and FPU
	mov %eax, %cr0

        fninit // initial fpu state
        fldcw _fpu_control_word

	lgdt gdt_desc
	ljmp $8, $start64

start32.no_multiboot:
        jmp start32.no_multiboot

start32.no_cpuid:
        jmp start32.no_cpuid

start32.no_ext_features:
        jmp start32.no_ext_features

start32.no_long_mode:
        jmp start32.no_long_mode

.code64
.align 4        
start64:
	.cfi_startproc simple
	.cfi_def_cfa %rsp, 0
	.cfi_undefined %rip
	mov $0x0, %ax
	mov %ax, %es
	mov %ax, %ss
	mov %ax, %ds
	mov %ax, %fs
	mov %ax, %gs
	lea boot_stack_top, %rsp
	call Main
	.cfi_endproc

#include "Smp.h"

.data
.global smpboot
smpboot:
.code16
        cli

        //set our data segment to the same as code segment
        mov %cs, %ax
        mov %ax, %ds
        //We can't use the original gdt pointer because it is too far away
        //remember this is 16 bit code so we can only access memory inside the
        // first 2^16 bytes which is beyond our original gdt pointer
        //This code gets moved so all offsets are relative to the beginning
        lgdtl smpboot_gdt_desc - smpboot

        //set PAE and PGE and OSXMMEXCPT and OSFXSR
        mov $0x6a0, %eax
        mov %eax, %cr4

        //point cr3 to boot page table
        mov $pml4, %eax
        mov %eax, %cr3

        //set EFER
        mov $0xc0000080, %ecx
        mov $0x900, %eax //NXE + LME
        xor %edx, %edx
        wrmsr

        //Activate longmode, enable paging and protection
        mov $0x80010023, %eax
        mov %eax, %cr0

        ljmp $0x8, $smpboot64 - smpboot + SMP_START_ADDRESS
.code64
smpboot64:
        .cfi_startproc simple
        .cfi_def_cfa %rsp, 0
        .cfi_undefined %rip
        mov $0x0, %ax
        mov %ax, %es
        mov %ax, %ss
        mov %ax, %ds
        mov %ax, %fs
        mov %ax, %gs
        fninit
        fldcw _fpu_control_word 
1:
        mov smp_stack_free - smpboot + SMP_START_ADDRESS, %rax
        mov (%rax), %rbx
        lock cmpxchg %rbx, smp_stack_free - smpboot + SMP_START_ADDRESS
        jnz 1b

        lea 4096(%rax), %rsp
        mov $SmpMain, %rax
        call *%rax
        .cfi_endproc

.global smp_stack_free
smp_stack_free:
        .quad 0
smpboot_gdt_desc:
        .short gdt_end - gdt - 1
        .long gdt

.global smpboot_end
smpboot_end = .
