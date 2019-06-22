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
#include <stddef.h>
#include <stdint.h>
#include "encoding.h"
#include "plic.h"
#include "syscalls.h"
#include "syslog.h"

volatile plic_t* const plic = (volatile plic_t*)PLIC_BASE_ADDR;

static plic_instance_t plic_instance[PLIC_NUM_CORES][IRQN_MAX];

void plic_init(void)
{
    int i = 0;

    /* Get current core id */
    unsigned long core_id = current_coreid();

    /* Disable all interrupts for the current core. */
    for (i = 0; i < ((PLIC_NUM_SOURCES + 32u) / 32u); i++)
        plic->target_enables.target[core_id].enable[i] = 0;

    static uint8_t s_plic_priorities_init_flag = 0;
    /* Set priorities to zero. */
    if(s_plic_priorities_init_flag == 0)
    {
        for (i = 0; i < PLIC_NUM_SOURCES; i++)
            plic->source_priorities.priority[i] = 0;
        s_plic_priorities_init_flag = 1;
    }

    /* Set the threshold to zero. */
    plic->targets.target[core_id].priority_threshold = 0;

    /* Clear PLIC instance for every cores */
    for (i = 0; i < IRQN_MAX; i++)
    {
        /* clang-format off */
        plic_instance[core_id][i] = (const plic_instance_t){
            .callback = NULL,
            .ctx      = NULL,
        };
        /* clang-format on */
    }

    /*
     * A successful claim will also atomically clear the corresponding
     * pending bit on the interrupt source. A target can perform a claim
     * at any time, even if the EIP is not set.
     */
    i = 0;
    while (plic->targets.target[core_id].claim_complete > 0 && i < 100)
    {
        /* This loop will clear pending bit on the interrupt source */
        i++;
    }

    /* Enable machine external interrupts. */
    set_csr(mie, MIP_MEIP);
}

int plic_irq_enable(plic_irq_t irq_number)
{
    /* Check parameters */
    if (PLIC_NUM_SOURCES < irq_number || 0 > irq_number)
        return -1;
    unsigned long core_id = current_coreid();
    /* Get current enable bit array by IRQ number */
    uint32_t current = plic->target_enables.target[core_id].enable[irq_number / 32];
    /* Set enable bit in enable bit array */
    current |= (uint32_t)1 << (irq_number % 32);
    /* Write back the enable bit array */
    plic->target_enables.target[core_id].enable[irq_number / 32] = current;
    return 0;
}

int plic_irq_disable(plic_irq_t irq_number)
{
    /* Check parameters */
    if (PLIC_NUM_SOURCES < irq_number || 0 > irq_number)
        return -1;
    unsigned long core_id = current_coreid();
    /* Get current enable bit array by IRQ number */
    uint32_t current = plic->target_enables.target[core_id].enable[irq_number / 32];
    /* Clear enable bit in enable bit array */
    current &= ~((uint32_t)1 << (irq_number % 32));
    /* Write back the enable bit array */
    plic->target_enables.target[core_id].enable[irq_number / 32] = current;
    return 0;
}

int plic_set_priority(plic_irq_t irq_number, uint32_t priority)
{
    /* Check parameters */
    if (PLIC_NUM_SOURCES < irq_number || 0 > irq_number)
        return -1;
    /* Set interrupt priority by IRQ number */
    plic->source_priorities.priority[irq_number] = priority;
    return 0;
}

uint32_t plic_get_priority(plic_irq_t irq_number)
{
    /* Check parameters */
    if (PLIC_NUM_SOURCES < irq_number || 0 > irq_number)
        return 0;
    /* Get interrupt priority by IRQ number */
    return plic->source_priorities.priority[irq_number];
}

uint32_t plic_irq_claim(void)
{
    unsigned long core_id = current_coreid();
    /* Perform IRQ claim */
    return plic->targets.target[core_id].claim_complete;
}

int plic_irq_complete(uint32_t source)
{
    unsigned long core_id = current_coreid();
    /* Perform IRQ complete */
    plic->targets.target[core_id].claim_complete = source;
    return 0;
}

void plic_irq_register(plic_irq_t irq, plic_irq_callback_t callback, void *ctx)
{
    /* Read core id */
    unsigned long core_id = current_coreid();
    /* Set user callback function */
    plic_instance[core_id][irq].callback = callback;
    /* Assign user context */
    plic_instance[core_id][irq].ctx = ctx;
}

void plic_irq_unregister(plic_irq_t irq)
{
    /* Just assign NULL to user callback function and context */
    plic_irq_register(irq, NULL, NULL);
}

void __attribute__((weak, alias("plic_irq_unregister"))) plic_irq_deregister(plic_irq_t irq);

plic_instance_t (*plic_get_instance(void))[IRQN_MAX]
{
    return plic_instance;
}

/*Entry Point for PLIC Interrupt Handler*/
uintptr_t __attribute__((weak))
handle_irq_m_ext(uintptr_t cause, uintptr_t epc)
{
    /*
     * After the highest-priority pending interrupt is claimed by a target
     * and the corresponding IP bit is cleared, other lower-priority
     * pending interrupts might then become visible to the target, and so
     * the PLIC EIP bit might not be cleared after a claim. The interrupt
     * handler can check the local meip/heip/seip/ueip bits before exiting
     * the handler, to allow more efficient service of other interrupts
     * without first restoring the interrupted context and taking another
     * interrupt trap.
     */
    if (read_csr(mip) & MIP_MEIP)
    {
        /* Get current core id */
        uint64_t core_id = current_coreid();
        /* Get primitive interrupt enable flag */
        uint64_t ie_flag = read_csr(mie);
        /* Get current IRQ num */
        uint32_t int_num = plic->targets.target[core_id].claim_complete;
        /* Get primitive IRQ threshold */
        uint32_t int_threshold = plic->targets.target[core_id].priority_threshold;
        /* Set new IRQ threshold = current IRQ threshold */
        plic->targets.target[core_id].priority_threshold = plic->source_priorities.priority[int_num];
        /* Disable software interrupt and timer interrupt */
        clear_csr(mie, MIP_MTIP | MIP_MSIP);
        /* Enable global interrupt */
        set_csr(mstatus, MSTATUS_MIE);
        if (plic_instance[core_id][int_num].callback)
            plic_instance[core_id][int_num].callback(
                plic_instance[core_id][int_num].ctx);
        /* Perform IRQ complete */
        plic->targets.target[core_id].claim_complete = int_num;
        /* Disable global interrupt */
        clear_csr(mstatus, MSTATUS_MIE);
        /* Set MPIE and MPP flag used to MRET instructions restore MIE flag */
        set_csr(mstatus, MSTATUS_MPIE | MSTATUS_MPP);
        /* Restore primitive interrupt enable flag */
        write_csr(mie, ie_flag);
        /* Restore primitive IRQ threshold */
        plic->targets.target[core_id].priority_threshold = int_threshold;
    }

    return epc;
}
