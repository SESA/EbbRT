/*
Optimized memcpy for x86-64.
 
Copyright (C) 2007-2014 Free Software Foundation, Inc.
Contributed by Evandro Menezes <evandro.menezes@amd.com>, 2007.
 
This file is part of the GNU C Library.
 
The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.
 
The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public
License along with the GNU C Library; if not, see
<http://www.gnu.org/licenses/>.
*/
 
/* Stack slots in the red-zone. */
 
#define RETVAL (-8)
#define SAVE0 (RETVAL - 8)
#define SAVE1 (SAVE0 - 8)
#define SAVE2 (SAVE1 - 8)
#define SAVE3 (SAVE2 - 8)
#define L(name) .L##name
.text

.globl memcpy
.type memcpy,@function
.align 1 << 4
memcpy:
.cfi_startproc        
/* Handle tiny blocks. */
 
L(1try): /* up to 32B */
	cmpq $32, %rdx
	movq %rdi, %rax /* save return value */
	jae L(1after)
 
L(1): /* 1-byte once */
	testb $1, %dl
	jz L(1a)
	 
	movzbl (%rsi), %ecx
	movb %cl, (%rdi)
	 
	incq %rsi
	incq %rdi
	 
.p2align 4,, 4
 
L(1a): /* 2-byte once */
	testb $2, %dl
	jz L(1b)
	 
	movzwl (%rsi), %ecx
	movw %cx, (%rdi)
	 
	addq $2, %rsi
	addq $2, %rdi
	 
.p2align 4,, 4
 
L(1b): /* 4-byte once */
	testb $4, %dl
	jz L(1c)
	 
	movl (%rsi), %ecx
	movl %ecx, (%rdi)
	 
	addq $4, %rsi
	addq $4, %rdi
	 
.p2align 4,, 4
 
L(1c): /* 8-byte once */
	testb $8, %dl
	jz L(1d)
	 
	movq (%rsi), %rcx
	movq %rcx, (%rdi)
	 
	addq $8, %rsi
	addq $8, %rdi
	 
.p2align 4,, 4
 
L(1d): /* 16-byte loop */
	andl $0xf0, %edx
	jz L(exit)
	 
.p2align 4
 
L(1loop):
	movq (%rsi), %rcx
	movq 8(%rsi), %r8
	movq %rcx, (%rdi)
	movq %r8, 8(%rdi)
	 
	subl $16, %edx
	 
	leaq 16(%rsi), %rsi
	leaq 16(%rdi), %rdi
	 
	jnz L(1loop)
	 
.p2align 4,, 4
 
L(exit): /* exit */
	rep
	retq
 
.p2align 4
 
L(1after):
        movq %rax, RETVAL(%rsp) /* save return value */
	 
	/* Align to the natural word size. */
	 
L(aligntry):
	movl %esi, %ecx /* align by source */
	 
	andl $7, %ecx
	jz L(alignafter) /* already aligned */
	 
L(align): /* align */
	leaq -8(%rcx, %rdx), %rdx /* calculate remaining bytes */
	subl $8, %ecx
 
.p2align 4
 
L(alignloop): /* 1-byte alignment loop */
	movzbl (%rsi), %eax
	movb %al, (%rdi)
	 
	incl %ecx
	 
	leaq 1(%rsi), %rsi
	leaq 1(%rdi), %rdi
	 
	jnz L(alignloop)
	 
.p2align 4
 
L(alignafter):
 
/* Handle mid-sized blocks. */
 
L(32try): /* up to 1KB */
	cmpq $1024, %rdx
	ja L(32after)
	 
L(32): /* 32-byte loop */
	movl %edx, %ecx
	shrl $5, %ecx
	jz L(32skip)
	 
.p2align 4
 
L(32loop):
	decl %ecx
	 
	movq (%rsi), %rax
	movq 8(%rsi), %r8
	movq 16(%rsi), %r9
	movq 24(%rsi), %r10
	 
	movq %rax, (%rdi)
	movq %r8, 8(%rdi)
	movq %r9, 16(%rdi)
	movq %r10, 24(%rdi)
	 
	leaq 32(%rsi), %rsi
	leaq 32(%rdi), %rdi
	 
	jz L(32skip) /* help out smaller blocks */
	 
	decl %ecx
	 
	movq (%rsi), %rax
	movq 8(%rsi), %r8
	movq 16(%rsi), %r9
	movq 24(%rsi), %r10
	 
	movq %rax, (%rdi)
	movq %r8, 8(%rdi)
	movq %r9, 16(%rdi)
	movq %r10, 24(%rdi)
	 
	leaq 32(%rsi), %rsi
	leaq 32(%rdi), %rdi
	 
	jnz L(32loop)
	 
.p2align 4
 
L(32skip):
        andl $31, %edx /* check for left overs */
	movq RETVAL(%rsp), %rax
	jnz L(1)
	 
	rep
	retq /* exit */
 
.p2align 4
 
L(32after):
 
/*
In order to minimize code-size in RTLD, algorithms specific for
larger blocks are excluded when building for RTLD.
*/
 
/* Handle blocks smaller than 1/2 L1. */
 
L(fasttry): /* first 1/2 L1 */
L(fast): /* good ol' MOVS */
	movq %rdx, %rcx
	shrq $3, %rcx
	jz L(fastskip)
	 
	rep
	movsq
	 
.p2align 4,, 4
 
L(fastskip):
	 
	andl $7, %edx /* check for left overs */
	movq RETVAL(%rsp), %rax
	jnz L(1)
	 
	rep
	retq /* exit */
	 
.cfi_endproc
.size memcpy,.-memcpy        
