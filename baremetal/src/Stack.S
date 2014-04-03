.global SwitchStack
SwitchStack:
        .cfi_startproc simple
        .cfi_def_cfa %rsp, 0
        .cfi_undefined %rip
        mov %rsi, %rsp
        callq *%rdx
        .cfi_endproc
	
.global SaveContextAndSwitch
SaveContextAndSwitch:
        .cfi_startproc simple
        .cfi_def_cfa %rsp, 0
        .cfi_undefined %rip
        mov %rbx, (%rcx)
        mov %rsp, 8(%rcx)
        mov %rbp, 16(%rcx)
        mov %r12, 24(%rcx)
	mov %r13, 32(%rcx)
	mov %r14, 40(%rcx)
	mov %r15, 48(%rcx)
        mov %rsi, %rsp             
        callq *%rdx               
	// will not get here  return happens at ActivateContextAndReturn
        .cfi_endproc               


.global ActivateContextAndReturn
ActivateContextAndReturn:
        mov (%rdi), %rbx
        mov 8(%rdi), %rsp
        mov 16(%rdi), %rbp
        mov 24(%rdi), %r12
        mov 32(%rdi), %r13
        mov 40(%rdi), %r14
        mov 48(%rdi), %r15
        ret

	
