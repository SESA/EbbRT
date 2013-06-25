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
.macro exception num:req
  .align 8
  .globl isr\num
  isr\num:
    jmp isr\num
.endm

.macro event num:req
  .align 8
  .globl isr\num
  isr\num:
    andq $(~(1 << 9)), 24(%rsp) //disable interrupt on iret
    pushq %rsp //store original rsp
    pushq (%rsp) //store it again right afterwards
    andq $-0x10, %rsp //align to 16 bytes
    movq $\num, %rdi
    call _event_interrupt
    //whether we were aligned or not, the original rsp is 8 bytes up
        //from our current stack pointer (because we pushed twice)
    movq 8(%rsp), %rsp
    iretq
.endm

.text
exception 0
exception 1
exception 2
exception 3
exception 4
exception 5
exception 6
exception 7
exception 8
exception 9
exception 10
exception 11
exception 12
exception 13
exception 14
exception 15
exception 16
exception 17
exception 18
exception 19
exception 20
exception 21
exception 22
exception 23
exception 24
exception 25
exception 26
exception 27
exception 28
exception 29
exception 30
exception 31
event 32
event 33
event 34
event 35
event 36
event 37
event 38
event 39
event 40
event 41
event 42
event 43
event 44
event 45
event 46
event 47
event 48
event 49
event 50
event 51
event 52
event 53
event 54
event 55
event 56
event 57
event 58
event 59
event 60
event 61
event 62
event 63
event 64
event 65
event 66
event 67
event 68
event 69
event 70
event 71
event 72
event 73
event 74
event 75
event 76
event 77
event 78
event 79
event 80
event 81
event 82
event 83
event 84
event 85
event 86
event 87
event 88
event 89
event 90
event 91
event 92
event 93
event 94
event 95
event 96
event 97
event 98
event 99
event 100
event 101
event 102
event 103
event 104
event 105
event 106
event 107
event 108
event 109
event 110
event 111
event 112
event 113
event 114
event 115
event 116
event 117
event 118
event 119
event 120
event 121
event 122
event 123
event 124
event 125
event 126
event 127
event 128
event 129
event 130
event 131
event 132
event 133
event 134
event 135
event 136
event 137
event 138
event 139
event 140
event 141
event 142
event 143
event 144
event 145
event 146
event 147
event 148
event 149
event 150
event 151
event 152
event 153
event 154
event 155
event 156
event 157
event 158
event 159
event 160
event 161
event 162
event 163
event 164
event 165
event 166
event 167
event 168
event 169
event 170
event 171
event 172
event 173
event 174
event 175
event 176
event 177
event 178
event 179
event 180
event 181
event 182
event 183
event 184
event 185
event 186
event 187
event 188
event 189
event 190
event 191
event 192
event 193
event 194
event 195
event 196
event 197
event 198
event 199
event 200
event 201
event 202
event 203
event 204
event 205
event 206
event 207
event 208
event 209
event 210
event 211
event 212
event 213
event 214
event 215
event 216
event 217
event 218
event 219
event 220
event 221
event 222
event 223
event 224
event 225
event 226
event 227
event 228
event 229
event 230
event 231
event 232
event 233
event 234
event 235
event 236
event 237
event 238
event 239
event 240
event 241
event 242
event 243
event 244
event 245
event 246
event 247
event 248
event 249
event 250
event 251
event 252
event 253
event 254
event 255
