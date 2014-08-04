/* memset/bzero -- set memory area to CH/0
Optimized version for x86-64.
Copyright (C) 2002-2014 Free Software Foundation, Inc.
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

.text
        
.globl memset
.type memset,@function
.align 1 << 4
memset:
.cfi_startproc
        movd %esi, %xmm8
        movq %rdi, %rax
        punpcklbw %xmm8, %xmm8
        punpcklwd %xmm8, %xmm8
        pshufd $0, %xmm8, %xmm8
.Lentry_from_bzero:
        cmpq $64, %rdx
        ja .Lloop_start
        cmpq $16, %rdx
        jbe .Lless_16_bytes
        cmpq $32, %rdx
        movdqu %xmm8, (%rdi)
        movdqu %xmm8, -16(%rdi,%rdx)
        ja .Lbetween_32_64_bytes
.Lreturn:
        rep
        ret
.p2align 4
.Lbetween_32_64_bytes:
        movdqu %xmm8, 16(%rdi)
        movdqu %xmm8, -32(%rdi,%rdx)
        ret
.p2align 4
.Lloop_start:
        leaq 64(%rdi), %rcx
        movdqu %xmm8, (%rdi)
        andq $-64, %rcx
        movdqu %xmm8, -16(%rdi,%rdx)
        movdqu %xmm8, 16(%rdi)
        movdqu %xmm8, -32(%rdi,%rdx)
        movdqu %xmm8, 32(%rdi)
        movdqu %xmm8, -48(%rdi,%rdx)
        movdqu %xmm8, 48(%rdi)
        movdqu %xmm8, -64(%rdi,%rdx)
        addq %rdi, %rdx
        andq $-64, %rdx
        cmpq %rdx, %rcx
        je .Lreturn
.p2align 4
.Lloop:
        movdqa %xmm8, (%rcx)
        movdqa %xmm8, 16(%rcx)
        movdqa %xmm8, 32(%rcx)
        movdqa %xmm8, 48(%rcx)
        addq $64, %rcx
        cmpq %rcx, %rdx
        jne .Lloop
        rep
        ret
.Lless_16_bytes:
        movq %xmm8, %rcx
        testb $24, %dl
        jne .Lbetween8_16bytes
        testb $4, %dl
        jne .Lbetween4_7bytes
        testb $1, %dl
        je .Lodd_byte
        movb %cl, (%rdi)
.Lodd_byte:
        testb $2, %dl
        je .Lreturn
        movw %cx, -2(%rax,%rdx)
        ret
.Lbetween4_7bytes:
        movl %ecx, (%rdi)
        movl %ecx, -4(%rdi,%rdx)
        ret
.Lbetween8_16bytes:
        movq %rcx, (%rdi)
        movq %rcx, -8(%rdi,%rdx)
        ret
.cfi_endproc
.size memset,.-memset        
