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

#include <syslog.h>
#include "timer.h"
#include "sysctl.h"
#include "stddef.h"
#include "utils.h"
#include "plic.h"
#include "io.h"

/**
 * @brief       Private definitions for the timer instance
 */
typedef struct timer_instance
{
    timer_callback_t callback;
    void *ctx;
    bool single_shot;
} timer_instance_t;

volatile timer_instance_t timer_instance[TIMER_DEVICE_MAX][TIMER_CHANNEL_MAX];

volatile kendryte_timer_t *const timer[3] =
{
    (volatile kendryte_timer_t *)TIMER0_BASE_ADDR,
    (volatile kendryte_timer_t *)TIMER1_BASE_ADDR,
    (volatile kendryte_timer_t *)TIMER2_BASE_ADDR
};

void timer_init(timer_device_number_t timer_number)
{
    for(size_t i = 0; i < TIMER_CHANNEL_MAX; i++)
        timer_instance[timer_number][i] = (const timer_instance_t) {
            .callback    = NULL,
            .ctx         = NULL,
            .single_shot = 0,
        };

    sysctl_clock_enable(SYSCTL_CLOCK_TIMER0 + timer_number);
}

void timer_set_clock_div(timer_device_number_t timer_number, uint32_t div)
{
    sysctl_clock_set_threshold(timer_number == 0 ? SYSCTL_THRESHOLD_TIMER0 :
        timer_number == 1 ? SYSCTL_THRESHOLD_TIMER1 :
        SYSCTL_THRESHOLD_TIMER2, div);
}

void timer_enable(timer_device_number_t timer_number, timer_channel_number_t channel)
{
    timer[timer_number]->channel[channel].control |= TIMER_CR_ENABLE;
}

void timer_disable(timer_device_number_t timer_number, timer_channel_number_t channel)
{
    timer[timer_number]->channel[channel].control &= (~TIMER_CR_ENABLE);
}

void timer_enable_pwm(timer_device_number_t timer_number, timer_channel_number_t channel)
{
    timer[timer_number]->channel[channel].control |= TIMER_CR_PWM_ENABLE;
}

void timer_disable_pwm(timer_device_number_t timer_number, timer_channel_number_t channel)
{
    timer[timer_number]->channel[channel].control &= (~TIMER_CR_PWM_ENABLE);
}

void timer_enable_interrupt(timer_device_number_t timer_number, timer_channel_number_t channel)
{
    timer[timer_number]->channel[channel].control &= (~TIMER_CR_INTERRUPT_MASK);
}

void timer_disable_interrupt(timer_device_number_t timer_number, timer_channel_number_t channel)
{
    timer[timer_number]->channel[channel].control |= TIMER_CR_INTERRUPT_MASK;
}

void timer_set_mode(timer_device_number_t timer_number, timer_channel_number_t channel, uint32_t mode)
{
    timer[timer_number]->channel[channel].control &= (~TIMER_CR_MODE_MASK);
    timer[timer_number]->channel[channel].control |= mode;
}

void timer_set_reload(timer_device_number_t timer_number, timer_channel_number_t channel, uint32_t count)
{
    timer[timer_number]->channel[channel].load_count = count;
}

void timer_set_reload2(timer_device_number_t timer_number, timer_channel_number_t channel, uint32_t count)
{
    timer[timer_number]->load_count2[channel] = count;
}

uint32_t timer_get_count(timer_device_number_t timer_number, timer_channel_number_t channel)
{
    return timer[timer_number]->channel[channel].current_value;
}

uint32_t timer_get_reload(timer_device_number_t timer_number, timer_channel_number_t channel)
{
    return timer[timer_number]->channel[channel].load_count;
}

uint32_t timer_get_reload2(timer_device_number_t timer_number, timer_channel_number_t channel)
{
    return timer[timer_number]->load_count2[channel];
}

uint32_t timer_get_interrupt_status(timer_device_number_t timer_number)
{
    return timer[timer_number]->intr_stat;
}

uint32_t timer_get_raw_interrupt_status(timer_device_number_t timer_number)
{
    return timer[timer_number]->raw_intr_stat;
}

uint32_t timer_channel_get_interrupt_status(timer_device_number_t timer_number, timer_channel_number_t channel)
{
    return timer[timer_number]->channel[channel].intr_stat;
}

void timer_clear_interrupt(timer_device_number_t timer_number)
{
    timer[timer_number]->eoi = timer[timer_number]->eoi;
}

void timer_channel_clear_interrupt(timer_device_number_t timer_number, timer_channel_number_t channel)
{
    timer[timer_number]->channel[channel].eoi = timer[timer_number]->channel[channel].eoi;
}

void timer_set_enable(timer_device_number_t timer_number, timer_channel_number_t channel, uint32_t enable)
{
    if (enable)
        timer[timer_number]->channel[channel].control = TIMER_CR_USER_MODE | TIMER_CR_ENABLE;
    else
        timer[timer_number]->channel[channel].control = TIMER_CR_INTERRUPT_MASK;
}

size_t timer_set_interval(timer_device_number_t timer_number, timer_channel_number_t channel, size_t nanoseconds)
{
    uint32_t clk_freq = sysctl_clock_get_freq(SYSCTL_CLOCK_TIMER0 + timer_number);

    double min_step = 1e9 / clk_freq;
    size_t value = (size_t)(nanoseconds / min_step);
    configASSERT(value > 0 && value < UINT32_MAX);
    timer[timer_number]->channel[channel].load_count = (uint32_t)value;
    return (size_t)(min_step * value);
}

typedef void(*timer_ontick)();
timer_ontick time_irq[3][4] = { NULL };

static int timer_isr(void *parm)
{
    uint32_t timer_number;
    for (timer_number = 0; timer_number < 3; timer_number++)
    {
        if (parm == timer[timer_number])
            break;
    }

    uint32_t channel = timer[timer_number]->intr_stat;
    size_t i = 0;
    for (i = 0; i < 4; i++)
    {
        if (channel & 1)
        {
            if (time_irq[timer_number][i])
                (time_irq[timer_number][i])();
            break;
        }

        channel >>= 1;
    }

    readl(&timer[timer_number]->eoi);
    return 0;
}

void timer_set_irq(timer_device_number_t timer_number, timer_channel_number_t channel, void(*func)(), uint32_t priority)
{
    time_irq[timer_number][channel] = func;
    if (channel < 2)
    {
        plic_set_priority(IRQN_TIMER0A_INTERRUPT + timer_number * 2, priority);
        plic_irq_register(IRQN_TIMER0A_INTERRUPT + timer_number * 2, timer_isr, (void *)timer[timer_number]);
        plic_irq_enable(IRQN_TIMER0A_INTERRUPT + timer_number * 2);
    }
    else
    {
        plic_set_priority(IRQN_TIMER0B_INTERRUPT + timer_number * 2, priority);
        plic_irq_register(IRQN_TIMER0B_INTERRUPT + timer_number * 2, timer_isr, (void *)timer[timer_number]);
        plic_irq_enable(IRQN_TIMER0B_INTERRUPT + timer_number * 2);
    }
}

/**
 * @brief             Get the timer irqn by device and channel object
 *
 * @note              Internal function, not public
 * @param  device     The device
 * @param  channel    The channel
 * @return plic_irq_t IRQ number
 */
static plic_irq_t get_timer_irqn_by_device_and_channel(timer_device_number_t device, timer_channel_number_t channel)
{
    if (device < TIMER_DEVICE_MAX && channel < TIMER_CHANNEL_MAX) {
        /*
         * Select timer interrupt part
         * Hierarchy of Timer interrupt to PLIC
         *  +---------+       +-----------+
         *  |        0+----+  |           |
         *  |         |    +--+0A         |
         *  |        1+----+  |           |
         *  | TIMER0  |       |           |
         *  |        2+----+  |           |
         *  |         |    +--+0B         |
         *  |        3+----+  |           |
         *  +---------+       |           |
         *                    |           |
         *  +---------+       |           |
         *  |        0+----+  |           |
         *  |         |    +--+1A         |
         *  |        1+----+  |           |
         *  | TIMER1  |       |    PLIC   |
         *  |        2+----+  |           |
         *  |         |    +--+1B         |
         *  |        3+----+  |           |
         *  +---------+       |           |
         *                    |           |
         *  +---------+       |           |
         *  |        0+----+  |           |
         *  |         |    +--+2A         |
         *  |        1+----+  |           |
         *  | TIMER2  |       |           |
         *  |        2+----+  |           |
         *  |         |    +--+2B         |
         *  |        3+----+  |           |
         *  +---------+       +-----------+
         *
         */
        if (channel < 2) {
            /* It is part A interrupt, offset + 0 */
            return IRQN_TIMER0A_INTERRUPT + device * 2;
        }
        else {
            /* It is part B interrupt, offset + 1 */
            return IRQN_TIMER0B_INTERRUPT + device * 2;
        }
    }
    return IRQN_NO_INTERRUPT;
}

/**
 * @brief         Process user callback function
 *
 * @note          Internal function, not public
 * @param  device The timer device
 * @param  ctx    The context
 * @return int    The callback result
 */
static int timer_interrupt_handler(timer_device_number_t device, void *ctx)
{
    uint32_t channel_int_stat = timer[device]->intr_stat;

    for (size_t i = 0; i < TIMER_CHANNEL_MAX; i++)
    {
        /* Check every bit for interrupt status */
        if (channel_int_stat & 1)
        {
            if (timer_instance[device][i].callback) {
                /* Process user callback function */
                timer_instance[device][i].callback(timer_instance[device][i].ctx);
                /* Check if this timer is a single shot timer */
                if (timer_instance[device][i].single_shot) {
                    /* Single shot timer, disable it */
                    timer_set_enable(device, i, 0);
                }
            }
            /* Clear timer interrupt flag for specific channel */
            readl(&timer[device]->channel[i].eoi);
        }
        channel_int_stat >>= 1;
    }

    /*
     * NOTE:
     * Don't read timer[device]->eoi here, or you will lost some interrupt
     * readl(&timer[device]->eoi);
     */

    return 0;
}

/**
 * @brief             Callback function bus for timer interrupt
 *
 * @note              Internal function, not public
 * @param  ctx        The context
 * @return int        The callback result
 */
static int timer0_interrupt_callback(void *ctx)
{
    return timer_interrupt_handler(TIMER_DEVICE_0, ctx);
}

/**
 * @brief             Callback function bus for timer interrupt
 *
 * @note              Internal function, not public
 * @param  ctx        The context
 * @return int        The callback result
 */
static int timer1_interrupt_callback(void *ctx)
{
    return timer_interrupt_handler(TIMER_DEVICE_1, ctx);
}

/**
 * @brief             Callback function bus for timer interrupt
 *
 * @note              Internal function, not public
 * @param  ctx        The context
 * @return int        The callback result
 */
static int timer2_interrupt_callback(void *ctx)
{
    return timer_interrupt_handler(TIMER_DEVICE_2, ctx);
}

int timer_irq_register(timer_device_number_t device, timer_channel_number_t channel, int is_single_shot, uint32_t priority, timer_callback_t callback, void *ctx)
{
    if (device < TIMER_DEVICE_MAX && channel < TIMER_CHANNEL_MAX) {
        plic_irq_t irq_number = get_timer_irqn_by_device_and_channel(device, channel);
        plic_irq_callback_t plic_irq_callback[TIMER_DEVICE_MAX] = {
            timer0_interrupt_callback,
            timer1_interrupt_callback,
            timer2_interrupt_callback,
        };

        timer_instance[device][channel] = (const timer_instance_t) {
            .callback    = callback,
            .ctx         = ctx,
            .single_shot = is_single_shot,
        };
        plic_set_priority(irq_number, priority);
        plic_irq_register(irq_number, plic_irq_callback[device], (void *)&timer_instance[device]);
        plic_irq_enable(irq_number);
        return 0;
    }
    return -1;
}

int timer_irq_unregister(timer_device_number_t device, timer_channel_number_t channel)
{
    if (device < TIMER_DEVICE_MAX && channel < TIMER_CHANNEL_MAX) {
        timer_instance[device][channel] = (const timer_instance_t) {
            .callback    = NULL,
            .ctx         = NULL,
            .single_shot = 0,
        };

        /* Combine 0 and 1 to A interrupt, 2 and 3 to B interrupt */
        if ((!(timer_instance[device][TIMER_CHANNEL_0].callback ||
              timer_instance[device][TIMER_CHANNEL_1].callback)) ||
            (!(timer_instance[device][TIMER_CHANNEL_2].callback ||
              timer_instance[device][TIMER_CHANNEL_3].callback))) {
            plic_irq_t irq_number = get_timer_irqn_by_device_and_channel(device, channel);
            plic_irq_unregister(irq_number);
        }
        return 0;
    }
    return -1;
}