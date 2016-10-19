/* Copyright (C) 2011-2014 Free Software Foundation, Inc.
Contributed by Intel Corporation.
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
<http://www.gnu.org/licenses/>. */
 
/* fast SSE2 version with using pmaxub and 64 byte loop */
 
.text

.globl memchr
.type memchr,@function
.align 1 << 4
memchr:
.cfi_startproc        
        movd %rsi, %xmm1
        mov %rdi, %rcx
 
        punpcklbw %xmm1, %xmm1
        test %rdx, %rdx
        jz .Lreturn_null
        punpcklbw %xmm1, %xmm1
 
        and $63, %rcx
        pshufd $0, %xmm1, %xmm1
 
        cmp $48, %rcx
        ja .Lcrosscache
 
        movdqu (%rdi), %xmm0
	pcmpeqb %xmm1, %xmm0
	pmovmskb %xmm0, %eax
        test %eax, %eax
 
	jnz .Lmatches_1
	sub $16, %rdx
	jbe .Lreturn_null
	add $16, %rdi
	and $15, %rcx
	and $-16, %rdi
	add %rcx, %rdx
	sub $64, %rdx
	jbe .Lexit_loop
	jmp .Lloop_prolog
 
.p2align 4
.Lcrosscache:
	and $15, %rcx
	and $-16, %rdi
	movdqa (%rdi), %xmm0
	 
	pcmpeqb %xmm1, %xmm0
	/* Check if there is a match. */
	pmovmskb %xmm0, %eax
	/* Remove the leading bytes. */
	sar %cl, %eax
	test %eax, %eax
	je .Lunaligned_no_match
	/* Check which byte is a match. */
	bsf %eax, %eax
	 
	sub %rax, %rdx
	jbe .Lreturn_null
	add %rdi, %rax
	add %rcx, %rax
	ret
 
.p2align 4
.Lunaligned_no_match:
	add %rcx, %rdx
	sub $16, %rdx
	jbe .Lreturn_null
	add $16, %rdi
	sub $64, %rdx
	jbe .Lexit_loop
 
.p2align 4
.Lloop_prolog:
	movdqa (%rdi), %xmm0
	pcmpeqb %xmm1, %xmm0
	pmovmskb %xmm0, %eax
	test %eax, %eax
	jnz .Lmatches
	 
	movdqa 16(%rdi), %xmm2
	pcmpeqb %xmm1, %xmm2
	pmovmskb %xmm2, %eax
	test %eax, %eax
	jnz .Lmatches16
	 
	movdqa 32(%rdi), %xmm3
	pcmpeqb %xmm1, %xmm3
	pmovmskb %xmm3, %eax
	test %eax, %eax
	jnz .Lmatches32
	 
	movdqa 48(%rdi), %xmm4
	pcmpeqb %xmm1, %xmm4
	add $64, %rdi
	pmovmskb %xmm4, %eax
	test %eax, %eax
	jnz .Lmatches0
	 
	test $0x3f, %rdi
	jz .Lalign64_loop
	 
	sub $64, %rdx
	jbe .Lexit_loop
	 
	movdqa (%rdi), %xmm0
	pcmpeqb %xmm1, %xmm0
	pmovmskb %xmm0, %eax
	test %eax, %eax
	jnz .Lmatches
	 
	movdqa 16(%rdi), %xmm2
	pcmpeqb %xmm1, %xmm2
	pmovmskb %xmm2, %eax
	test %eax, %eax
	jnz .Lmatches16
	 
	movdqa 32(%rdi), %xmm3
	pcmpeqb %xmm1, %xmm3
	pmovmskb %xmm3, %eax
	test %eax, %eax
	jnz .Lmatches32
	 
	movdqa 48(%rdi), %xmm3
	pcmpeqb %xmm1, %xmm3
	pmovmskb %xmm3, %eax
	 
	add $64, %rdi
	test %eax, %eax
	jnz .Lmatches0
	 
	mov %rdi, %rcx
	and $-64, %rdi
	and $63, %rcx
	add %rcx, %rdx
	 
.p2align 4
.Lalign64_loop:
	sub $64, %rdx
	jbe .Lexit_loop
	movdqa (%rdi), %xmm0
	movdqa 16(%rdi), %xmm2
	movdqa 32(%rdi), %xmm3
	movdqa 48(%rdi), %xmm4
	 
	pcmpeqb %xmm1, %xmm0
	pcmpeqb %xmm1, %xmm2
	pcmpeqb %xmm1, %xmm3
	pcmpeqb %xmm1, %xmm4
	 
	pmaxub %xmm0, %xmm3
	pmaxub %xmm2, %xmm4
	pmaxub %xmm3, %xmm4
	pmovmskb %xmm4, %eax
	 
	add $64, %rdi
	 
	test %eax, %eax
	jz .Lalign64_loop
	 
	sub $64, %rdi
	 
	pmovmskb %xmm0, %eax
	test %eax, %eax
	jnz .Lmatches
	 
	pmovmskb %xmm2, %eax
	test %eax, %eax
	jnz .Lmatches16
	 
	movdqa 32(%rdi), %xmm3
	pcmpeqb %xmm1, %xmm3
	 
	pcmpeqb 48(%rdi), %xmm1
	pmovmskb %xmm3, %eax
	test %eax, %eax
	jnz .Lmatches32
	 
	pmovmskb %xmm1, %eax
	bsf %eax, %eax
	lea 48(%rdi, %rax), %rax
	ret
	 
.p2align 4
.Lexit_loop:
	add $32, %rdx
	jle .Lexit_loop_32
	 
	movdqa (%rdi), %xmm0
	pcmpeqb %xmm1, %xmm0
	pmovmskb %xmm0, %eax
	test %eax, %eax
	jnz .Lmatches
	 
	movdqa 16(%rdi), %xmm2
	pcmpeqb %xmm1, %xmm2
	pmovmskb %xmm2, %eax
	test %eax, %eax
	jnz .Lmatches16
	 
	movdqa 32(%rdi), %xmm3
	pcmpeqb %xmm1, %xmm3
	pmovmskb %xmm3, %eax
	test %eax, %eax
	jnz .Lmatches32_1
	sub $16, %rdx
	jle .Lreturn_null
	 
	pcmpeqb 48(%rdi), %xmm1
	pmovmskb %xmm1, %eax
	test %eax, %eax
	jnz .Lmatches48_1
	xor %rax, %rax
	ret
	 
.p2align 4
.Lexit_loop_32:
	add $32, %rdx
	movdqa (%rdi), %xmm0
	pcmpeqb %xmm1, %xmm0
	pmovmskb %xmm0, %eax
	test %eax, %eax
	jnz .Lmatches_1
	sub $16, %rdx
	jbe .Lreturn_null
	 
	pcmpeqb 16(%rdi), %xmm1
	pmovmskb %xmm1, %eax
	test %eax, %eax
	jnz .Lmatches16_1
	xor %rax, %rax
	ret
	 
.p2align 4
.Lmatches0:
	bsf %eax, %eax
	lea -16(%rax, %rdi), %rax
	ret
	 
.p2align 4
.Lmatches:
	bsf %eax, %eax
	add %rdi, %rax
	ret
	 
.p2align 4
.Lmatches16:
	bsf %eax, %eax
	lea 16(%rax, %rdi), %rax
	ret
	 
.p2align 4
.Lmatches32:
	bsf %eax, %eax
	lea 32(%rax, %rdi), %rax
	ret
	 
.p2align 4
.Lmatches_1:
	bsf %eax, %eax
	sub %rax, %rdx
	jbe .Lreturn_null
	add %rdi, %rax
	ret
	 
.p2align 4
.Lmatches16_1:
	bsf %eax, %eax
	sub %rax, %rdx
	jbe .Lreturn_null
	lea 16(%rdi, %rax), %rax
	ret
	 
.p2align 4
.Lmatches32_1:
	bsf %eax, %eax
	sub %rax, %rdx
	jbe .Lreturn_null
	lea 32(%rdi, %rax), %rax
	ret
	 
.p2align 4
.Lmatches48_1:
	bsf %eax, %eax
	sub %rax, %rdx
	jbe .Lreturn_null
	lea 48(%rdi, %rax), %rax
	ret
	 
.p2align 4
.Lreturn_null:
	xor %rax, %rax
	ret
.cfi_endproc
.size memchr,.-memchr        
