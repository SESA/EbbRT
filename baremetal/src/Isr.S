.macro PUSHQ_CFI reg
        pushq \reg
        .cfi_adjust_cfa_offset 8
        .cfi_rel_offset \reg, 0
.endm

.macro POPQ_CFI reg
        popq \reg
        .cfi_adjust_cfa_offset -8
        .cfi_restore \reg
.endm

.macro INTERRUPT_ENTRY name, handler, has_error_code, fpu
        .align 8
        .global \name
        \name :
        .cfi_startproc simple
        .cfi_signal_frame
        .if \has_error_code == 0
        pushq $0
        .endif
        .cfi_def_cfa %rsp, 0
        .cfi_offset %rip, 8
        .cfi_offset %rsp, 32
        PUSHQ_CFI %rax
        PUSHQ_CFI %rbx
        PUSHQ_CFI %rcx
        PUSHQ_CFI %rdx
        PUSHQ_CFI %rsi
        PUSHQ_CFI %rdi
        PUSHQ_CFI %rbp
        PUSHQ_CFI %r8
        PUSHQ_CFI %r9
        PUSHQ_CFI %r10
        PUSHQ_CFI %r11
        PUSHQ_CFI %r12
        PUSHQ_CFI %r13
        PUSHQ_CFI %r14
        PUSHQ_CFI %r15
        sub $520, %rsp
        .cfi_adjust_cfa_offset 520
        .if \fpu == 1
        fwait
        fxsave (%rsp)
        .endif
        mov %rsp, %rdi
        call \handler
        .if \fpu == 1
        fxrstor (%rsp)
        .endif
        add $520, %rsp
        .cfi_adjust_cfa_offset -520
        POPQ_CFI %r15
        POPQ_CFI %r14
        POPQ_CFI %r13
        POPQ_CFI %r12
        POPQ_CFI %r11
        POPQ_CFI %r10
        POPQ_CFI %r9
        POPQ_CFI %r8
        POPQ_CFI %rbp
        POPQ_CFI %rdi
        POPQ_CFI %rsi
        POPQ_CFI %rdx
        POPQ_CFI %rcx
        POPQ_CFI %rbx
        POPQ_CFI %rax
        add $8, %rsp
        iretq
        .cfi_endproc
.endm

.macro INTERRUPT_ERROR_ENTRY name, handler
        INTERRUPT_ENTRY \name, \handler, 1, 1
.endm

.macro INTERRUPT_NOERROR_ENTRY name, handler
        INTERRUPT_ENTRY \name, \handler, 0, 1
.endm

.cfi_sections .eh_frame, .debug_frame

INTERRUPT_NOERROR_ENTRY IntDe, DivideErrorException
INTERRUPT_NOERROR_ENTRY IntDb, DebugException
INTERRUPT_NOERROR_ENTRY IntNmi, NmiInterrupt
INTERRUPT_NOERROR_ENTRY IntBp, BreakpointException
INTERRUPT_NOERROR_ENTRY IntOf, OverflowException
INTERRUPT_NOERROR_ENTRY IntBr, BoundRangeExceededException
INTERRUPT_NOERROR_ENTRY IntUd, InvalidOpcodeException
INTERRUPT_NOERROR_ENTRY IntNm, DeviceNotAvailableException
INTERRUPT_ERROR_ENTRY IntDf, DoubleFaultException
INTERRUPT_ERROR_ENTRY IntTs, InvalidTssException
INTERRUPT_ERROR_ENTRY IntNp, SegmentNotPresent
INTERRUPT_ERROR_ENTRY IntSs, StackFaultException
INTERRUPT_ERROR_ENTRY IntGp, GeneralProtectionException
INTERRUPT_ERROR_ENTRY IntPf, PageFaultException
INTERRUPT_ENTRY IntMf, X87FpuFloatingPointError, 0, 0
INTERRUPT_ERROR_ENTRY IntAc, AlignmentCheckException
INTERRUPT_NOERROR_ENTRY IntMc, MachineCheckException
INTERRUPT_NOERROR_ENTRY IntXm, SimdFloatingPointException

.align 16
.globl EventEntry
EventEntry:
num = 32
.rept 256 - 32
        .align 16
        movq $num, %rdi
        jmp EventEntryCommon
        num = num + 1
.endr

EventEntryCommon:
        .cfi_startproc simple
        .cfi_undefined %rip
        pushq %rsp
        pushq (%rsp)
        andq $-0x10, %rsp
        call EventInterrupt
        .cfi_endproc
