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
#include "arch/powerpc64/asm.h"

//    define default functions
.macro default_func num:req
	.align 3
        .globl default_func\num
default_func\num:
         // Three word function descriptor
	.quad .L.default_func\num //address of entry point
        .quad .TOC.@tocbase //TOC base
        .quad 0 //environment pointer
.L.default_func\num:
        li r11, \num
        b default_func_common
.endm

      	.section ".text"

default_func_common:
        .cfi_startproc
        .cfi_personality 0x94,DW.ref.__gxx_personality_v0
        .cfi_lsda 0x14,.LLSDA0
        mflr 0
        std 0, 0x10(1) //save link register
        stdu 1,-(0x70+(18*8))(1) //push frame
        .cfi_def_cfa_offset (0x70+(18*8))
        .cfi_offset 65, 16 //where LR can be found

        //save parameter gprs
	std 10, ((0x70+(18*8))+0x68)(1)
        std 9, ((0x70+(18*8))+0x60)(1)
        std 8, ((0x70+(18*8))+0x58)(1)
        std 7, ((0x70+(18*8))+0x50)(1)
        std 6, ((0x70+(18*8))+0x48)(1)
        std 5, ((0x70+(18*8))+0x40)(1)
        std 4, ((0x70+(18*8))+0x38)(1)
        std 3, ((0x70+(18*8))+0x30)(1)

	//save parameter fprs
        stfd 13, ((0x70+(18*8)) + ((-1)*8))(1)
        stfd 12, ((0x70+(18*8)) + ((-2)*8))(1)
        stfd 11, ((0x70+(18*8)) + ((-3)*8))(1)
        stfd 10, ((0x70+(18*8)) + ((-4)*8))(1)
        stfd 9, ((0x70+(18*8)) + ((-5)*8))(1)
        stfd 8, ((0x70+(18*8)) + ((-6)*8))(1)
        stfd 7, ((0x70+(18*8)) + ((-7)*8))(1)
        stfd 6, ((0x70+(18*8)) + ((-8)*8))(1)
        stfd 5, ((0x70+(18*8)) + ((-9)*8))(1)
        stfd 4, ((0x70+(18*8)) + ((-10)*8))(1)
        stfd 3, ((0x70+(18*8)) + ((-11)*8))(1)
        stfd 2, ((0x70+(18*8)) + ((-12)*8))(1)
        stfd 1, ((0x70+(18*8)) + ((-13)*8))(1)

        //store pointer to stack args
        la 3, ((0x70+(18*8)) + 0x70)(1)
	std 3, ((0x70+(18*8)) + ((-14)*8))(1)

        //store pointer to fprs
        la 3, ((-13)*8)(1)
	std 3, ((0x70+(18*8)) + ((-15)*8))(1)

        //store pointer to gprs
        la 3, ((0x70+(18*8))+0x30)(1)
	std 3, ((0x70+(18*8)) + ((-16)*8))(1)

        la 3, ((0x70+(18*8)) + ((-16)*8))(1) //first param: args pointer
        mr 4, 11 //second param: method number
	la 5, ((0x70+(18*8)) + ((-17)*8))(1) 	//third param: address to FuncRet
.Lpre_precall:
        bl _trans_precall
        nop
.Lpost_precall:

        cmpdi 3, 0
        bne .Ldocall
        ld 3, ((0x70+18*8) + ((-17*8)))(1)
        addi 1, 1, (0x70 + (18*8)) //restore stack pointer
        .cfi_def_cfa_offset 0
        ld  0, 16(1)
        mtlr 0
        .cfi_restore 65
        blr
        .p2align 4,,15
.Ldocall:
        .cfi_def_cfa_offset (0x70+(18*8))
        .cfi_offset 65, 16 //where LR can be found
        ld 3, ((0x70+(18*8)) + 0x10)(1) //load link register
        bl _event_altstack_push
        nop

        mr 3, 2
        bl _event_altstack_push //store toc pointer
        nop 
        ld 3, ((0x70+(18*8)) + ((-17)*8))(1)
        ld 2, 0x8(3) //load new toc pointer
        ld 3, 0(3) //load function address
        mtctr 3

        //restore parameter gprs
	ld 10, ((0x70+(18*8))+0x68)(1)
        ld 9, ((0x70+(18*8))+0x60)(1)
        ld 8, ((0x70+(18*8))+0x58)(1)
        ld 7, ((0x70+(18*8))+0x50)(1)
        ld 6, ((0x70+(18*8))+0x48)(1)
        ld 5, ((0x70+(18*8))+0x40)(1)
        ld 4, ((0x70+(18*8))+0x38)(1)
        ld 3, ((0x70+(18*8))+0x30)(1)

	//save parameter fprs
        lfd 13, ((0x70+(18*8)) + ((-1)*8))(1)
        lfd 12, ((0x70+(18*8)) + ((-2)*8))(1)
        lfd 11, ((0x70+(18*8)) + ((-3)*8))(1)
        lfd 10, ((0x70+(18*8)) + ((-4)*8))(1)
        lfd 9, ((0x70+(18*8)) + ((-5)*8))(1)
        lfd 8, ((0x70+(18*8)) + ((-6)*8))(1)
        lfd 7, ((0x70+(18*8)) + ((-7)*8))(1)
        lfd 6, ((0x70+(18*8)) + ((-8)*8))(1)
        lfd 5, ((0x70+(18*8)) + ((-9)*8))(1)
        lfd 4, ((0x70+(18*8)) + ((-10)*8))(1)
        lfd 3, ((0x70+(18*8)) + ((-11)*8))(1)
        lfd 2, ((0x70+(18*8)) + ((-12)*8))(1)
        lfd 1, ((0x70+(18*8)) + ((-13)*8))(1)

        //restore stack pointer
        la 1, (0x70+(18*8))(1)
        .cfi_def_cfa_offset 0
.Lpre_foo_call:
        bctrl
.Lpost_foo_call:
        //move stack pointer back
        la 1, -(0x70+(18*8))(1)
        .cfi_def_cfa_offset (0x70 + (18*8))
        std 3, ((0x70+(18*8)) + (-0x8))(1) //store return value

        bl _event_altstack_pop
        nop

        mr 2, 3 //restore toc pointer
              
        bl _event_altstack_pop
        nop

   	mtlr 3 // load the link register with the original value
	.cfi_restore 65 //we restored the link register
        ld 3, ((0x70+(18*8)) + (-0x8))(1) //load return value
        la 1, (0x70+(18*8))(1) //restore stack pointer
        .cfi_def_cfa_offset 0
        b _trans_postcall // this should return to the original caller
.Llp0:
        stdu 1, -0x70(1)
        .cfi_def_cfa_offset 0x70
        bl __cxa_begin_catch
        nop
        bl _event_altstack_pop
        nop
        mr 2,3 //restore toc pointer
        bl _event_altstack_pop
        nop
        std 3, (0x70 + 0x10)(1) //store link register
        .cfi_offset 65, 16 //where LR can be found
.Lpre_rethrow:
        bl __cxa_rethrow
        nop
.Lpost_rethrow:
        std 3, (0x70 - 0x8)(1)
        bl __cxa_end_catch
        nop
        ld 3, (0x70 - 0x8)(1)
.Lpre_unwind:
        bl _Unwind_Resume
        nop
.Lpost_unwind:
        //PPC64 Traceback table
	.long 0 //Marks beginning of traceback table
	.byte 0 //version
        .byte 12 //language: assembly
        .byte 0
        .byte 1 //saves_lr (LR is saved in LR save area)
        .byte 128 //stores_bc (back chain is saved in stack frame)
        .byte 0
        .byte 0
        .byte 0
        .cfi_endproc
.LFE0:
        .globl __gxx_personality_v0
        .section        .gcc_except_table,"a",@progbits
        .align 3
.LLSDA0: //Language specific data area for exception handling
        .byte 0xff //DW_EH_PE_omit (no encoding of landing pad base)
        .byte 0x04 //DW_EH_PE_udata8 encoding of entries in type
        //table
        .uleb128 (.LLSDATT0 - . ) //types table distance from here
        .byte 0x01 //DW_EH_PE_uleb128 (encoding of call-site table)
        .uleb128 (.LLSDACSE0 - .LLSDACSB0) //length of call-site table
.LLSDACSB0:
        .uleb128 (.Lpre_precall - default_func_common)
        .uleb128 (.Lpost_precall - .Lpre_precall)
        .uleb128 0x00
        .uleb128 0x00

        .uleb128 (.Lpre_foo_call - default_func_common) //offset to call site
        .uleb128 (.Lpost_foo_call - .Lpre_foo_call) //length
        .uleb128 (.Llp0 - default_func_common) //offset to landing pad
        .uleb128 (1) //1 plus byte offset into action table

        .uleb128 (.Lpre_rethrow - default_func_common)
        .uleb128 (.Lpost_rethrow - .Lpre_rethrow)
        .uleb128 (.Lpost_rethrow - default_func_common)
        .uleb128 0x00 //no action

        .uleb128 (.Lpre_unwind - default_func_common)
        .uleb128 (.Lpost_unwind - .Lpre_unwind)
        .uleb128 0x00 //no landing pad, just resume
        .uleb128 0x00
.LLSDACSE0:
        //action table
        .byte 0x1 //offset into types table
        .byte 0x0 //no further actions


       .quad 0x0
.LLSDATT0:
              
	.section	".text"
	.hidden	DW.ref.__gxx_personality_v0
	.weak	DW.ref.__gxx_personality_v0
	.section	.data.DW.ref.__gxx_personality_v0,"awG",@progbits,DW.ref.__gxx_personality_v0,comdat
	.align 3
	.type	DW.ref.__gxx_personality_v0, @object
	.size	DW.ref.__gxx_personality_v0, 8
	DW.ref.__gxx_personality_v0:
	.quad	__gxx_personality_v0
	.ident	"GCC: (GNU) 4.8.1"

        .section ".text"

default_func 0
default_func 1
default_func 2
default_func 3
default_func 4
default_func 5
default_func 6
default_func 7
default_func 8
default_func 9

default_func 10
default_func 11
default_func 12
default_func 13
default_func 14
default_func 15
default_func 16
default_func 17
default_func 18
default_func 19

default_func 20
default_func 21
default_func 22
default_func 23
default_func 24
default_func 25
default_func 26
default_func 27
default_func 28
default_func 29

default_func 30
default_func 31
default_func 32
default_func 33
default_func 34
default_func 35
default_func 36
default_func 37
default_func 38
default_func 39

default_func 40
default_func 41
default_func 42
default_func 43
default_func 44
default_func 45
default_func 46
default_func 47
default_func 48
default_func 49

default_func 50
default_func 51
default_func 52
default_func 53
default_func 54
default_func 55
default_func 56
default_func 57
default_func 58
default_func 59

default_func 60
default_func 61
default_func 62
default_func 63
default_func 64
default_func 65
default_func 66
default_func 67
default_func 68
default_func 69

default_func 70
default_func 71
default_func 72
default_func 73
default_func 74
default_func 75
default_func 76
default_func 77
default_func 78
default_func 79

default_func 80
default_func 81
default_func 82
default_func 83
default_func 84
default_func 85
default_func 86
default_func 87
default_func 88
default_func 89

default_func 90
default_func 91
default_func 92
default_func 93
default_func 94
default_func 95
default_func 96
default_func 97
default_func 98
default_func 99

default_func 100
default_func 101
default_func 102
default_func 103
default_func 104
default_func 105
default_func 106
default_func 107
default_func 108
default_func 109

default_func 110
default_func 111
default_func 112
default_func 113
default_func 114
default_func 115
default_func 116
default_func 117
default_func 118
default_func 119

default_func 120
default_func 121
default_func 122
default_func 123
default_func 124
default_func 125
default_func 126
default_func 127
default_func 128
default_func 129

default_func 130
default_func 131
default_func 132
default_func 133
default_func 134
default_func 135
default_func 136
default_func 137
default_func 138
default_func 139

default_func 140
default_func 141
default_func 142
default_func 143
default_func 144
default_func 145
default_func 146
default_func 147
default_func 148
default_func 149

default_func 150
default_func 151
default_func 152
default_func 153
default_func 154
default_func 155
default_func 156
default_func 157
default_func 158
default_func 159

default_func 160
default_func 161
default_func 162
default_func 163
default_func 164
default_func 165
default_func 166
default_func 167
default_func 168
default_func 169

default_func 170
default_func 171
default_func 172
default_func 173
default_func 174
default_func 175
default_func 176
default_func 177
default_func 178
default_func 179

default_func 180
default_func 181
default_func 182
default_func 183
default_func 184
default_func 185
default_func 186
default_func 187
default_func 188
default_func 189

default_func 190
default_func 191
default_func 192
default_func 193
default_func 194
default_func 195
default_func 196
default_func 197
default_func 198
default_func 199

default_func 200
default_func 201
default_func 202
default_func 203
default_func 204
default_func 205
default_func 206
default_func 207
default_func 208
default_func 209

default_func 210
default_func 211
default_func 212
default_func 213
default_func 214
default_func 215
default_func 216
default_func 217
default_func 218
default_func 219

default_func 220
default_func 221
default_func 222
default_func 223
default_func 224
default_func 225
default_func 226
default_func 227
default_func 228
default_func 229

default_func 230
default_func 231
default_func 232
default_func 233
default_func 234
default_func 235
default_func 236
default_func 237
default_func 238
default_func 239

default_func 240
default_func 241
default_func 242
default_func 243
default_func 244
default_func 245
default_func 246
default_func 247
default_func 248
default_func 249

default_func 250
default_func 251
default_func 252
default_func 253
default_func 254
default_func 255
