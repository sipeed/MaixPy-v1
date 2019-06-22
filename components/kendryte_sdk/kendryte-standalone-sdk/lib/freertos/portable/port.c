/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
* FreeRTOS Kernel V10.0.1
* Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* http://www.FreeRTOS.org
* http://aws.amazon.com/freertos
*
* 1 tab == 4 spaces!
*/

/*-----------------------------------------------------------
 * Implementation of functions defined in portable.h for the RISC-V port.
 *----------------------------------------------------------*/

/* Scheduler includes. */
#include <atomic.h>
#include <clint.h>
#include <encoding.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysctl.h>
#include <syslog.h>
#include "core_sync.h"
#include "FreeRTOS.h"
#include "portmacro.h"
#include "task.h"

/* A variable is used to keep track of the critical section nesting.  This
variable has to be stored as part of the task context and must be initialised to
a non zero value to ensure interrupts don't inadvertently become unmasked before
the scheduler starts.  As it is stored as part of the task context it will
automatically be set to 0 when the first task is started. */
static UBaseType_t uxCriticalNesting[portNUM_PROCESSORS] = {[0 ... portNUM_PROCESSORS - 1] = 0xaaaaaaaa};
PRIVILEGED_DATA static corelock_t xCoreLock = CORELOCK_INIT;

/* Contains context when starting scheduler, save all 31 registers */
#ifdef __gracefulExit
#error Not ported
BaseType_t xStartContext[31] = {0};
#endif

/*
 * Handler for timer interrupt
 */
void vPortSysTickHandler(void);

/*
 * Setup the timer to generate the tick interrupts.
 */
void vPortSetupTimer(void);

/*
 * Used to catch tasks that attempt to return from their implementing function.
 */
static void prvTaskExitError(void);

UBaseType_t uxPortGetProcessorId()
{
    return (UBaseType_t)read_csr(mhartid);
}

/*-----------------------------------------------------------*/

/* Sets and enable the timer interrupt */
void vPortSetupTimer(void)
{
    UBaseType_t uxPsrId = uxPortGetProcessorId();
    clint->mtimecmp[uxPsrId] = clint->mtime + (configTICK_CLOCK_HZ / configTICK_RATE_HZ);
    /* Enable timer and soft interupt */
    __asm volatile("csrs mie,%0" ::"r"(0x88));
}

/*-----------------------------------------------------------*/

/* Sets the next timer interrupt
 * Reads previous timer compare register, and adds tickrate */
void prvSetNextTimerInterrupt(void)
{
    UBaseType_t uxPsrId = uxPortGetProcessorId();
    clint->mtimecmp[uxPsrId] += (configTICK_CLOCK_HZ / configTICK_RATE_HZ);
}
/*-----------------------------------------------------------*/

void prvTaskExitError(void)
{
    /* A function that implements a task must not exit or attempt to return to
    its caller as there is nothing to return to.  If a task wants to exit it
    should instead call vTaskDelete( NULL ).

    Artificially force an assert() to be triggered if configASSERT() is
    defined, then stop here so application writers can catch the error. */
    UBaseType_t uxPsrId = uxPortGetProcessorId();
    configASSERT(uxCriticalNesting[uxPsrId] == ~0UL);
    portDISABLE_INTERRUPTS();
    for (;;)
        ;
}
/*-----------------------------------------------------------*/

/* Clear current interrupt mask and set given mask */
void vPortClearInterruptMask(int mask)
{
    __asm volatile("csrw mie, %0" ::"r"(mask));
}
/*-----------------------------------------------------------*/

/* Set interrupt mask and return current interrupt enable register */
int vPortSetInterruptMask(void)
{
    int ret;
    __asm volatile("csrr %0,mie"
                   : "=r"(ret));
    __asm volatile("csrc mie,%0" ::"i"(7));
    return ret;
}
/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
StackType_t* pxPortInitialiseStack(StackType_t* pxTopOfStack, TaskFunction_t pxCode, void* pvParameters)
{
    /* Simulate the stack frame as it would be created by a context switch
	interrupt. */
    pxTopOfStack -= 64;
    memset(pxTopOfStack, 0, sizeof(StackType_t) * 64);

    pxTopOfStack[0] = (portSTACK_TYPE)prvTaskExitError; /* Register ra */
    pxTopOfStack[1] = (portSTACK_TYPE)pxTopOfStack;
    pxTopOfStack[8] = (portSTACK_TYPE)pvParameters; /* Register a0 */
    pxTopOfStack[30] = 0; /* Register fsr */
    pxTopOfStack[31] = (portSTACK_TYPE)pxCode; /* Register mepc */

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/
void vPortSysTickHandler(void)
{
    core_sync_complete_context_switch(uxPortGetProcessorId());
    vTaskSwitchContext();
}
/*-----------------------------------------------------------*/

void vPortEnterCritical(void)
{
    if (!core_sync_is_in_progress(uxPortGetProcessorId()))
        vTaskEnterCritical();
    corelock_lock(&xCoreLock);
}

void vPortExitCritical(void)
{
    corelock_unlock(&xCoreLock);
    if (!core_sync_is_in_progress(uxPortGetProcessorId()))
        vTaskExitCritical();
}

void vPortYield()
{
    core_sync_request_context_switch(uxPortGetProcessorId());
}

void vPortFatal(const char* file, int line, const char* message)
{
    portDISABLE_INTERRUPTS();
    corelock_lock(&xCoreLock);
    LOGE("FreeRTOS", "(%s:%d) %s", file, line, message);
    exit(-1);
    while (1)
        ;
}

UBaseType_t uxPortGetCPUClock()
{
    return (UBaseType_t)sysctl_cpu_get_freq();
}
