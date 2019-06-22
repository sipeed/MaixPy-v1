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
#include <atomic.h>
#include <clint.h>
#include <core_sync.h>
#include <encoding.h>
#include <plic.h>
#include "FreeRTOS.h"
#include "task.h"

volatile UBaseType_t g_core_pending_switch[portNUM_PROCESSORS] = { 0 };
static volatile UBaseType_t s_core_sync_events[portNUM_PROCESSORS] = { 0 };
static volatile TaskHandle_t s_pending_to_add_tasks[portNUM_PROCESSORS] = { 0 };
static volatile UBaseType_t s_core_sync_in_progress[portNUM_PROCESSORS] = { 0 };

uintptr_t handle_irq_m_soft(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32])
{
    uint64_t core_id = uxPortGetProcessorId();
    atomic_set(&s_core_sync_in_progress[core_id], 1);
    switch (s_core_sync_events[core_id])
    {
    case CORE_SYNC_ADD_TCB:
    {
        TaskHandle_t newTask = atomic_read(&s_pending_to_add_tasks[core_id]);
        if (newTask)
        {
            vAddNewTaskToCurrentReadyList(newTask);
            atomic_set(&s_pending_to_add_tasks[core_id], NULL);
        }
    }
    break;
    default:
        break;
    }

    core_sync_complete(core_id);
    return epc;
}

uintptr_t handle_irq_m_timer(uintptr_t cause, uintptr_t epc, uintptr_t regs[32], uintptr_t fregs[32])
{
    prvSetNextTimerInterrupt();

    /* Increment the RTOS tick. */
    if (xTaskIncrementTick() != pdFALSE)
    {
        core_sync_request_context_switch(uxPortGetProcessorId());
    }

    return epc;
}

void core_sync_request_context_switch(uint64_t core_id)
{
    atomic_set(&g_core_pending_switch[core_id], 1);
    clint_ipi_send(core_id);
}

void core_sync_complete(uint64_t core_id)
{
    if (atomic_read(&g_core_pending_switch[core_id]) == 0)
        clint_ipi_clear(core_id);
    atomic_set(&s_core_sync_events[core_id], CORE_SYNC_NONE);
    atomic_set(&s_core_sync_in_progress[core_id], 0);
}

void core_sync_complete_context_switch(uint64_t core_id)
{
    if (atomic_read(&s_core_sync_events[core_id]) == CORE_SYNC_NONE)
        clint_ipi_clear(core_id);
    atomic_set(&g_core_pending_switch[core_id], 0);
}

int core_sync_is_in_progress(uint64_t core_id)
{
    return !!atomic_read(&s_core_sync_in_progress[core_id]);
}

void vPortAddNewTaskToReadyListAsync(UBaseType_t uxPsrId, void* pxNewTaskHandle)
{
    // Wait for last adding tcb complete
    while (atomic_read(&s_pending_to_add_tasks[uxPsrId]));
    atomic_set(&s_pending_to_add_tasks[uxPsrId], pxNewTaskHandle);

    while (atomic_cas(&s_core_sync_events[uxPsrId], CORE_SYNC_NONE, CORE_SYNC_ADD_TCB) != CORE_SYNC_NONE)
        ;
    clint_ipi_send(uxPsrId);
}
