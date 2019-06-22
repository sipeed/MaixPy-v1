# Copyright 2018 Canaan Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#include "encoding.h"

# define LREG ld
# define SREG sd
# define LFREG flw
# define SFREG fsw
# define REGBYTES 8
# define STKSHIFT 15


.section .text.start, "ax", @progbits
.globl _start
_start:
  j 1f
  .word 0xdeadbeef
  .align 3
  .global g_wake_up
  g_wake_up:
      .dword 1
      .dword 0
1:
  csrw mideleg, 0
  csrw medeleg, 0
  csrw mie, 0
  csrw mip, 0
  la t0, trap_entry
  csrw mtvec, t0

  li  x1, 0
  li  x2, 0
  li  x3, 0
  li  x4, 0
  li  x5, 0
  li  x6, 0
  li  x7, 0
  li  x8, 0
  li  x9, 0
  li  x10,0
  li  x11,0
  li  x12,0
  li  x13,0
  li  x14,0
  li  x15,0
  li  x16,0
  li  x17,0
  li  x18,0
  li  x19,0
  li  x20,0
  li  x21,0
  li  x22,0
  li  x23,0
  li  x24,0
  li  x25,0
  li  x26,0
  li  x27,0
  li  x28,0
  li  x29,0
  li  x30,0
  li  x31,0

  li t0, MSTATUS_FS
  csrs mstatus, t0

  fssr    x0
  fmv.w.x f0, x0
  fmv.w.x f1, x0
  fmv.w.x f2, x0
  fmv.w.x f3, x0
  fmv.w.x f4, x0
  fmv.w.x f5, x0
  fmv.w.x f6, x0
  fmv.w.x f7, x0
  fmv.w.x f8, x0
  fmv.w.x f9, x0
  fmv.w.x f10,x0
  fmv.w.x f11,x0
  fmv.w.x f12,x0
  fmv.w.x f13,x0
  fmv.w.x f14,x0
  fmv.w.x f15,x0
  fmv.w.x f16,x0
  fmv.w.x f17,x0
  fmv.w.x f18,x0
  fmv.w.x f19,x0
  fmv.w.x f20,x0
  fmv.w.x f21,x0
  fmv.w.x f22,x0
  fmv.w.x f23,x0
  fmv.w.x f24,x0
  fmv.w.x f25,x0
  fmv.w.x f26,x0
  fmv.w.x f27,x0
  fmv.w.x f28,x0
  fmv.w.x f29,x0
  fmv.w.x f30,x0
  fmv.w.x f31,x0

  .option push
  .option norelax
   la gp, __global_pointer$
  .option pop
  la  tp, _end + 63
  and tp, tp, -64
  csrr a0, mhartid

  add sp, a0, 1
  sll sp, sp, STKSHIFT
  add sp, sp, tp

  j _init_bsp

  .globl trap_entry
  .type trap_entry, @function
  .align 2
trap_entry:
  addi sp, sp, -REGBYTES
  sd t0, 0x0(sp)
  csrr t0, mcause
  bgez t0, .handle_other
  # Test soft interrupt
  slli t0, t0, 1
  addi t0, t0, -(IRQ_M_SOFT << 1)
  bnez t0, .handle_other
  # Interupt is soft interrupt
  # Get event
  addi sp, sp, -REGBYTES
  sd t1, 0x0(sp)
  la   t0, g_core_pending_switch
  csrr t1, mhartid
  slli t1, t1, 3
  add  t0, t0, t1
  ld t1, 0x0(sp)
  addi sp, sp, REGBYTES
  # Test ContextSwitch event
  ld   t0, 0x0(t0)
  beqz t0, .handle_other

  ld t0, 0x0(sp)
  addi sp, sp, REGBYTES
  # Do not use jal here
  j    xPortSysTickInt
  mret
.handle_other:
  ld   t0, 0x0(sp)
  addi sp, sp, REGBYTES
  addi sp, sp, -64*REGBYTES

  SREG x1, 1*REGBYTES(sp)
  SREG x2, 2*REGBYTES(sp)
  SREG x3, 3*REGBYTES(sp)
  SREG x4, 4*REGBYTES(sp)
  SREG x5, 5*REGBYTES(sp)
  SREG x6, 6*REGBYTES(sp)
  SREG x7, 7*REGBYTES(sp)
  SREG x8, 8*REGBYTES(sp)
  SREG x9, 9*REGBYTES(sp)
  SREG x10, 10*REGBYTES(sp)
  SREG x11, 11*REGBYTES(sp)
  SREG x12, 12*REGBYTES(sp)
  SREG x13, 13*REGBYTES(sp)
  SREG x14, 14*REGBYTES(sp)
  SREG x15, 15*REGBYTES(sp)
  SREG x16, 16*REGBYTES(sp)
  SREG x17, 17*REGBYTES(sp)
  SREG x18, 18*REGBYTES(sp)
  SREG x19, 19*REGBYTES(sp)
  SREG x20, 20*REGBYTES(sp)
  SREG x21, 21*REGBYTES(sp)
  SREG x22, 22*REGBYTES(sp)
  SREG x23, 23*REGBYTES(sp)
  SREG x24, 24*REGBYTES(sp)
  SREG x25, 25*REGBYTES(sp)
  SREG x26, 26*REGBYTES(sp)
  SREG x27, 27*REGBYTES(sp)
  SREG x28, 28*REGBYTES(sp)
  SREG x29, 29*REGBYTES(sp)
  SREG x30, 30*REGBYTES(sp)
  SREG x31, 31*REGBYTES(sp)

  SFREG f0, ( 0  + 32)*REGBYTES(sp)
  SFREG f1, ( 1  + 32)*REGBYTES(sp)
  SFREG f2, ( 2  + 32)*REGBYTES(sp)
  SFREG f3, ( 3  + 32)*REGBYTES(sp)
  SFREG f4, ( 4  + 32)*REGBYTES(sp)
  SFREG f5, ( 5  + 32)*REGBYTES(sp)
  SFREG f6, ( 6  + 32)*REGBYTES(sp)
  SFREG f7, ( 7  + 32)*REGBYTES(sp)
  SFREG f8, ( 8  + 32)*REGBYTES(sp)
  SFREG f9, ( 9  + 32)*REGBYTES(sp)
  SFREG f10,( 10 + 32)*REGBYTES(sp)
  SFREG f11,( 11 + 32)*REGBYTES(sp)
  SFREG f12,( 12 + 32)*REGBYTES(sp)
  SFREG f13,( 13 + 32)*REGBYTES(sp)
  SFREG f14,( 14 + 32)*REGBYTES(sp)
  SFREG f15,( 15 + 32)*REGBYTES(sp)
  SFREG f16,( 16 + 32)*REGBYTES(sp)
  SFREG f17,( 17 + 32)*REGBYTES(sp)
  SFREG f18,( 18 + 32)*REGBYTES(sp)
  SFREG f19,( 19 + 32)*REGBYTES(sp)
  SFREG f20,( 20 + 32)*REGBYTES(sp)
  SFREG f21,( 21 + 32)*REGBYTES(sp)
  SFREG f22,( 22 + 32)*REGBYTES(sp)
  SFREG f23,( 23 + 32)*REGBYTES(sp)
  SFREG f24,( 24 + 32)*REGBYTES(sp)
  SFREG f25,( 25 + 32)*REGBYTES(sp)
  SFREG f26,( 26 + 32)*REGBYTES(sp)
  SFREG f27,( 27 + 32)*REGBYTES(sp)
  SFREG f28,( 28 + 32)*REGBYTES(sp)
  SFREG f29,( 29 + 32)*REGBYTES(sp)
  SFREG f30,( 30 + 32)*REGBYTES(sp)
  SFREG f31,( 31 + 32)*REGBYTES(sp)

  csrr a0, mcause
  csrr a1, mepc
  mv a2, sp
  add a3, sp, 32*REGBYTES
  bgez a0, .handle_syscall
.handle_irq:
  jal handle_irq
  j .restore
.handle_syscall:
  jal handle_syscall
.restore:
  csrw mepc, a0
  LREG x1, 1*REGBYTES(sp)
  LREG x2, 2*REGBYTES(sp)
  LREG x3, 3*REGBYTES(sp)
  LREG x4, 4*REGBYTES(sp)
  LREG x5, 5*REGBYTES(sp)
  LREG x6, 6*REGBYTES(sp)
  LREG x7, 7*REGBYTES(sp)
  LREG x8, 8*REGBYTES(sp)
  LREG x9, 9*REGBYTES(sp)
  LREG x10, 10*REGBYTES(sp)
  LREG x11, 11*REGBYTES(sp)
  LREG x12, 12*REGBYTES(sp)
  LREG x13, 13*REGBYTES(sp)
  LREG x14, 14*REGBYTES(sp)
  LREG x15, 15*REGBYTES(sp)
  LREG x16, 16*REGBYTES(sp)
  LREG x17, 17*REGBYTES(sp)
  LREG x18, 18*REGBYTES(sp)
  LREG x19, 19*REGBYTES(sp)
  LREG x20, 20*REGBYTES(sp)
  LREG x21, 21*REGBYTES(sp)
  LREG x22, 22*REGBYTES(sp)
  LREG x23, 23*REGBYTES(sp)
  LREG x24, 24*REGBYTES(sp)
  LREG x25, 25*REGBYTES(sp)
  LREG x26, 26*REGBYTES(sp)
  LREG x27, 27*REGBYTES(sp)
  LREG x28, 28*REGBYTES(sp)
  LREG x29, 29*REGBYTES(sp)
  LREG x30, 30*REGBYTES(sp)
  LREG x31, 31*REGBYTES(sp)

  LFREG f0, ( 0  + 32)*REGBYTES(sp)
  LFREG f1, ( 1  + 32)*REGBYTES(sp)
  LFREG f2, ( 2  + 32)*REGBYTES(sp)
  LFREG f3, ( 3  + 32)*REGBYTES(sp)
  LFREG f4, ( 4  + 32)*REGBYTES(sp)
  LFREG f5, ( 5  + 32)*REGBYTES(sp)
  LFREG f6, ( 6  + 32)*REGBYTES(sp)
  LFREG f7, ( 7  + 32)*REGBYTES(sp)
  LFREG f8, ( 8  + 32)*REGBYTES(sp)
  LFREG f9, ( 9  + 32)*REGBYTES(sp)
  LFREG f10,( 10 + 32)*REGBYTES(sp)
  LFREG f11,( 11 + 32)*REGBYTES(sp)
  LFREG f12,( 12 + 32)*REGBYTES(sp)
  LFREG f13,( 13 + 32)*REGBYTES(sp)
  LFREG f14,( 14 + 32)*REGBYTES(sp)
  LFREG f15,( 15 + 32)*REGBYTES(sp)
  LFREG f16,( 16 + 32)*REGBYTES(sp)
  LFREG f17,( 17 + 32)*REGBYTES(sp)
  LFREG f18,( 18 + 32)*REGBYTES(sp)
  LFREG f19,( 19 + 32)*REGBYTES(sp)
  LFREG f20,( 20 + 32)*REGBYTES(sp)
  LFREG f21,( 21 + 32)*REGBYTES(sp)
  LFREG f22,( 22 + 32)*REGBYTES(sp)
  LFREG f23,( 23 + 32)*REGBYTES(sp)
  LFREG f24,( 24 + 32)*REGBYTES(sp)
  LFREG f25,( 25 + 32)*REGBYTES(sp)
  LFREG f26,( 26 + 32)*REGBYTES(sp)
  LFREG f27,( 27 + 32)*REGBYTES(sp)
  LFREG f28,( 28 + 32)*REGBYTES(sp)
  LFREG f29,( 29 + 32)*REGBYTES(sp)
  LFREG f30,( 30 + 32)*REGBYTES(sp)
  LFREG f31,( 31 + 32)*REGBYTES(sp)

  addi sp, sp, 64*REGBYTES
  mret

.section ".tdata.begin"
.globl _tdata_begin
_tdata_begin:

.section ".tdata.end"
.globl _tdata_end
_tdata_end:

.section ".tbss.end"
.globl _tbss_end
_tbss_end:

