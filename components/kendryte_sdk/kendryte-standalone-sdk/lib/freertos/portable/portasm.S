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
#
# FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
# All rights reserved

# VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

# This file is part of the FreeRTOS distribution and was contributed
# to the project by Technolution B.V. (www.technolution.nl,
# freertos-riscv@technolution.eu) under the terms of the FreeRTOS
# contributors license.

# FreeRTOS is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License (version 2) as published by the
# Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

# ***************************************************************************
# >>!   NOTE: The modification to the GPL is included to allow you to     !<<
# >>!   distribute a combined work that includes FreeRTOS without being   !<<
# >>!   obliged to provide the source code for proprietary components     !<<
# >>!   outside of the FreeRTOS kernel.                                   !<<
# ***************************************************************************

# FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  Full license text is available on the following
# link: http://www.freertos.org/a00114.html

# ***************************************************************************
#  *                                                                       *
#  *    FreeRTOS provides completely free yet professionally developed,    *
#  *    robust, strictly quality controlled, supported, and cross          *
#  *    platform software that is more than just the market leader, it     *
#  *    is the industry's de facto standard.                               *
#  *                                                                       *
#  *    Help yourself get started quickly while simultaneously helping     *
#  *    to support the FreeRTOS project by purchasing a FreeRTOS           *
#  *    tutorial book, reference manual, or both:                          *
#  *    http://www.FreeRTOS.org/Documentation                              *
#  *                                                                       *
# ***************************************************************************

# http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
# the FAQ page "My application does not run, what could be wrong?".  Have you
# defined configASSERT()?

# http://www.FreeRTOS.org/support - In return for receiving this top quality
# embedded software for free we request you assist our global community by
# participating in the support forum.

# http://www.FreeRTOS.org/training - Investing in training allows your team to
# be as productive as possible as early as possible.  Now you can receive
# FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
# Ltd, and the world's leading authority on the world's leading RTOS.

# http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
# including FreeRTOS+Trace - an indispensable productivity tool, a DOS
# compatible FAT file system, and our tiny thread aware UDP/IP stack.

# http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
# Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

# http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
# Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
# licenses offer ticketed support, indemnification and commercial middleware.

# http://www.SafeRTOS.com - High Integrity Systems also provide a safety
# engineered and independently SIL3 certified version for use in safety and
# mission critical applications that require provable dependability.

# 1 tab == 4 spaces!
#

# include "encoding.h"

# define REGBYTES 8

.global xPortSysTickInt
.global xPortStartScheduler
.global vPortYield
.global vTaskIncrementTick
.global vPortEndScheduler
.global xExitStack
.global uxPortGetProcessorId

# /* Macro for saving task context */
.macro portSAVE_CONTEXT
  .global pxCurrentTCB
  # /* make room in stack */
  addi sp, sp, -REGBYTES * 64

  # /* Save Context */
  sd ra,   0 * REGBYTES(sp)
  sd sp,   1 * REGBYTES(sp)
  sd tp,   2 * REGBYTES(sp)
  sd t0,   3 * REGBYTES(sp)
  sd t1,   4 * REGBYTES(sp)
  sd t2,   5 * REGBYTES(sp)
  sd s0,   6 * REGBYTES(sp)
  sd s1,   7 * REGBYTES(sp)
  sd a0,   8 * REGBYTES(sp)
  sd a1,   9 * REGBYTES(sp)
  sd a2,  10 * REGBYTES(sp)
  sd a3,  11 * REGBYTES(sp)
  sd a4,  12 * REGBYTES(sp)
  sd a5,  13 * REGBYTES(sp)
  sd a6,  14 * REGBYTES(sp)
  sd a7,  15 * REGBYTES(sp)
  sd s2,  16 * REGBYTES(sp)
  sd s3,  17 * REGBYTES(sp)
  sd s4,  18 * REGBYTES(sp)
  sd s5,  19 * REGBYTES(sp)
  sd s6,  20 * REGBYTES(sp)
  sd s7,  21 * REGBYTES(sp)
  sd s8,  22 * REGBYTES(sp)
  sd s9,  23 * REGBYTES(sp)
  sd s10, 24 * REGBYTES(sp)
  sd s11, 25 * REGBYTES(sp)
  sd t3,  26 * REGBYTES(sp)
  sd t4,  27 * REGBYTES(sp)
  sd t5,  28 * REGBYTES(sp)
  sd t6,  29 * REGBYTES(sp)
  
  frsr t0
  sd t0, 30 * REGBYTES(sp)

  csrr t0, mepc
  sd t0, 31 * REGBYTES(sp)

  fsd f0,  ( 0 + 32) * REGBYTES(sp)
  fsd f1,  ( 1 + 32) * REGBYTES(sp)
  fsd f2,  ( 2 + 32) * REGBYTES(sp)
  fsd f3,  ( 3 + 32) * REGBYTES(sp)
  fsd f4,  ( 4 + 32) * REGBYTES(sp)
  fsd f5,  ( 5 + 32) * REGBYTES(sp)
  fsd f6,  ( 6 + 32) * REGBYTES(sp)
  fsd f7,  ( 7 + 32) * REGBYTES(sp)
  fsd f8,  ( 8 + 32) * REGBYTES(sp)
  fsd f9,  ( 9 + 32) * REGBYTES(sp)
  fsd f10, (10 + 32) * REGBYTES(sp)
  fsd f11, (11 + 32) * REGBYTES(sp)
  fsd f12, (12 + 32) * REGBYTES(sp)
  fsd f13, (13 + 32) * REGBYTES(sp)
  fsd f14, (14 + 32) * REGBYTES(sp)
  fsd f15, (15 + 32) * REGBYTES(sp)
  fsd f16, (16 + 32) * REGBYTES(sp)
  fsd f17, (17 + 32) * REGBYTES(sp)
  fsd f18, (18 + 32) * REGBYTES(sp)
  fsd f19, (19 + 32) * REGBYTES(sp)
  fsd f20, (20 + 32) * REGBYTES(sp)
  fsd f21, (21 + 32) * REGBYTES(sp)
  fsd f22, (22 + 32) * REGBYTES(sp)
  fsd f23, (23 + 32) * REGBYTES(sp)
  fsd f24, (24 + 32) * REGBYTES(sp)
  fsd f25, (25 + 32) * REGBYTES(sp)
  fsd f26, (26 + 32) * REGBYTES(sp)
  fsd f27, (27 + 32) * REGBYTES(sp)
  fsd f28, (28 + 32) * REGBYTES(sp)
  fsd f29, (29 + 32) * REGBYTES(sp)
  fsd f30, (30 + 32) * REGBYTES(sp)
  fsd f31, (31 + 32) * REGBYTES(sp)

  # /* Store current stackpointer in task control block (TCB) */
  la t0, pxCurrentTCB
  csrr t1, mhartid
  slli t1, t1, 3
  add t0, t0, t1

  ld t0, 0x0(t0)
  sd sp, 0x0(t0)
  .endm

# /* Macro for restoring task context */
.macro portRESTORE_CONTEXT
  .global pxCurrentTCB
  # /* Load stack pointer from the current TCB */
  la t0, pxCurrentTCB
  csrr t1, mhartid
  slli t1, t1, 3
  add t0, t0, t1

  ld sp, 0x0(t0)
  ld sp, 0x0(sp)

  ld t0, 30 * REGBYTES(sp)
  fssr t0

  ld t0, 31 * REGBYTES(sp)
  csrw mepc, t0

  # /* Run in machine mode */
  li t0, MSTATUS_MPP | MSTATUS_MPIE
  csrs mstatus, t0

  # /* Restore registers */
  ld ra,   0 * REGBYTES(sp)
  ld sp,   1 * REGBYTES(sp)
  ld tp,   2 * REGBYTES(sp)
  ld t0,   3 * REGBYTES(sp)
  ld t1,   4 * REGBYTES(sp)
  ld t2,   5 * REGBYTES(sp)
  ld s0,   6 * REGBYTES(sp)
  ld s1,   7 * REGBYTES(sp)
  ld a0,   8 * REGBYTES(sp)
  ld a1,   9 * REGBYTES(sp)
  ld a2,  10 * REGBYTES(sp)
  ld a3,  11 * REGBYTES(sp)
  ld a4,  12 * REGBYTES(sp)
  ld a5,  13 * REGBYTES(sp)
  ld a6,  14 * REGBYTES(sp)
  ld a7,  15 * REGBYTES(sp)
  ld s2,  16 * REGBYTES(sp)
  ld s3,  17 * REGBYTES(sp)
  ld s4,  18 * REGBYTES(sp)
  ld s5,  19 * REGBYTES(sp)
  ld s6,  20 * REGBYTES(sp)
  ld s7,  21 * REGBYTES(sp)
  ld s8,  22 * REGBYTES(sp)
  ld s9,  23 * REGBYTES(sp)
  ld s10, 24 * REGBYTES(sp)
  ld s11, 25 * REGBYTES(sp)
  ld t3,  26 * REGBYTES(sp)
  ld t4,  27 * REGBYTES(sp)
  ld t5,  28 * REGBYTES(sp)
  ld t6,  29 * REGBYTES(sp)

  fld f0,  ( 0 + 32) * REGBYTES(sp)
  fld f1,  ( 1 + 32) * REGBYTES(sp)
  fld f2,  ( 2 + 32) * REGBYTES(sp)
  fld f3,  ( 3 + 32) * REGBYTES(sp)
  fld f4,  ( 4 + 32) * REGBYTES(sp)
  fld f5,  ( 5 + 32) * REGBYTES(sp)
  fld f6,  ( 6 + 32) * REGBYTES(sp)
  fld f7,  ( 7 + 32) * REGBYTES(sp)
  fld f8,  ( 8 + 32) * REGBYTES(sp)
  fld f9,  ( 9 + 32) * REGBYTES(sp)
  fld f10, (10 + 32) * REGBYTES(sp)
  fld f11, (11 + 32) * REGBYTES(sp)
  fld f12, (12 + 32) * REGBYTES(sp)
  fld f13, (13 + 32) * REGBYTES(sp)
  fld f14, (14 + 32) * REGBYTES(sp)
  fld f15, (15 + 32) * REGBYTES(sp)
  fld f16, (16 + 32) * REGBYTES(sp)
  fld f17, (17 + 32) * REGBYTES(sp)
  fld f18, (18 + 32) * REGBYTES(sp)
  fld f19, (19 + 32) * REGBYTES(sp)
  fld f20, (20 + 32) * REGBYTES(sp)
  fld f21, (21 + 32) * REGBYTES(sp)
  fld f22, (22 + 32) * REGBYTES(sp)
  fld f23, (23 + 32) * REGBYTES(sp)
  fld f24, (24 + 32) * REGBYTES(sp)
  fld f25, (25 + 32) * REGBYTES(sp)
  fld f26, (26 + 32) * REGBYTES(sp)
  fld f27, (27 + 32) * REGBYTES(sp)
  fld f28, (28 + 32) * REGBYTES(sp)
  fld f29, (29 + 32) * REGBYTES(sp)
  fld f30, (30 + 32) * REGBYTES(sp)
  fld f31, (31 + 32) * REGBYTES(sp)

  addi    sp, sp, REGBYTES * 64
  mret
  .endm

xPortStartScheduler:
  jal     vPortSetupTimer
  portRESTORE_CONTEXT

vPortEndScheduler:
  ret
  
.section .text.systick, "ax", @progbits
xPortSysTickInt:
  portSAVE_CONTEXT
  call vPortSysTickHandler
  portRESTORE_CONTEXT