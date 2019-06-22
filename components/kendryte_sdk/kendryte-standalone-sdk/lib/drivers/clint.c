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
#include "clint.h"
#include "sysctl.h"

volatile clint_t* const clint = (volatile clint_t*)CLINT_BASE_ADDR;
static clint_timer_instance_t clint_timer_instance[CLINT_NUM_CORES];
static clint_ipi_instance_t clint_ipi_instance[CLINT_NUM_CORES];

uint64_t clint_get_time(void)
{
    /* No difference on cores */
    return clint->mtime;
}

int clint_timer_init(void)
{
    /* Read core id */
    unsigned long core_id = current_coreid();
    /* Clear the Machine-Timer bit in MIE */
    clear_csr(mie, MIP_MTIP);
    /* Fill core's instance with original data */

    /* clang-format off */
    clint_timer_instance[core_id] = (const clint_timer_instance_t)
    {
        .interval    = 0,
        .cycles      = 0,
        .single_shot = 0,
        .callback    = NULL,
        .ctx         = NULL,
    };
    /* clang-format on */

    return 0;
}

int clint_timer_stop(void)
{
    /* Clear the Machine-Timer bit in MIE */
    clear_csr(mie, MIP_MTIP);
    return 0;
}

uint64_t clint_timer_get_freq(void)
{
    /* The clock is divided by CLINT_CLOCK_DIV */
    return sysctl_clock_get_freq(SYSCTL_CLOCK_CPU) / CLINT_CLOCK_DIV;
}

int clint_timer_start(uint64_t interval, int single_shot)
{
    /* Read core id */
    unsigned long core_id = current_coreid();
    /* Set timer interval */
    if (clint_timer_set_interval(interval) != 0)
        return -1;
    /* Set timer single shot */
    if (clint_timer_set_single_shot(single_shot) != 0)
        return -1;
    /* Check settings to prevent interval is 0 */
    if (clint_timer_instance[core_id].interval == 0)
        return -1;
    /* Check settings to prevent cycles is 0 */
    if (clint_timer_instance[core_id].cycles == 0)
        return -1;
    /* Add cycle interval to mtimecmp */
    uint64_t now = clint->mtime;
    uint64_t then = now + clint_timer_instance[core_id].cycles;
    /* Set mtimecmp by core id */
    clint->mtimecmp[core_id] = then;
    /* Enable interrupts in general */
    set_csr(mstatus, MSTATUS_MIE);
    /* Enable the Machine-Timer bit in MIE */
    set_csr(mie, MIP_MTIP);
    return 0;
}

uint64_t clint_timer_get_interval(void)
{
    /* Read core id */
    unsigned long core_id = current_coreid();
    return clint_timer_instance[core_id].interval;
}

int clint_timer_set_interval(uint64_t interval)
{
    /* Read core id */
    unsigned long core_id = current_coreid();
    /* Check parameter */
    if (interval == 0)
        return -1;

    /* Assign user interval with Millisecond(ms) */
    clint_timer_instance[core_id].interval = interval;
    /* Convert interval to cycles */
    clint_timer_instance[core_id].cycles = interval * clint_timer_get_freq() / 1000ULL;
    return 0;
}

int clint_timer_get_single_shot(void)
{
    /* Read core id */
    unsigned long core_id = current_coreid();
    /* Get single shot mode by core id */
    return clint_timer_instance[core_id].single_shot;
}

int clint_timer_set_single_shot(int single_shot)
{
    /* Read core id */
    unsigned long core_id = current_coreid();
    /* Set single shot mode by core id */
    clint_timer_instance[core_id].single_shot = single_shot;
    return 0;
}

int clint_timer_register(clint_timer_callback_t callback, void *ctx)
{
    /* Read core id */
    unsigned long core_id = current_coreid();
    /* Set user callback function */
    clint_timer_instance[core_id].callback = callback;
    /* Assign user context */
    clint_timer_instance[core_id].ctx = ctx;
    return 0;
}

int clint_timer_unregister(void)
{
    /* Just assign NULL to user callback function and context */
    return clint_timer_register(NULL, NULL);
}

int clint_ipi_init(void)
{
    /* Read core id */
    unsigned long core_id = current_coreid();
    /* Clear the Machine-Software bit in MIE */
    clear_csr(mie, MIP_MSIP);
    /* Fill core's instance with original data */
    /* clang-format off */
    clint_ipi_instance[core_id] = (const clint_ipi_instance_t){
        .callback    = NULL,
        .ctx         = NULL,
    };
    /* clang-format on */

    return 0;
}

int clint_ipi_enable(void)
{
    /* Enable interrupts in general */
    set_csr(mstatus, MSTATUS_MIE);
    /* Set the Machine-Software bit in MIE */
    set_csr(mie, MIP_MSIP);
    return 0;
}

int clint_ipi_disable(void)
{
    /* Clear the Machine-Software bit in MIE */
    clear_csr(mie, MIP_MSIP);
    return 0;
}

int clint_ipi_send(size_t core_id)
{
    if (core_id >= CLINT_NUM_CORES)
        return -1;
    clint->msip[core_id].msip = 1;
    return 0;
}

int clint_ipi_clear(size_t core_id)
{
    if (core_id >= CLINT_NUM_CORES)
        return -1;
    if (clint->msip[core_id].msip)
    {
        clint->msip[core_id].msip = 0;
        return 1;
    }
    return 0;
}

int clint_ipi_register(clint_ipi_callback_t callback, void *ctx)
{
    /* Read core id */
    unsigned long core_id = current_coreid();
    /* Set user callback function */
    clint_ipi_instance[core_id].callback = callback;
    /* Assign user context */
    clint_ipi_instance[core_id].ctx = ctx;
    return 0;
}

int clint_ipi_unregister(void)
{
    /* Just assign NULL to user callback function and context */
    return clint_ipi_register(NULL, NULL);
}

